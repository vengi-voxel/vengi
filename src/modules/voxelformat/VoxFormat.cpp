/**
 * @file
 */

#include "VoxFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Color.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/UTF8.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"
#include <SDL_assert.h>
#include <glm/gtc/matrix_access.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

#define wrapFree(read, memory) \
	if ((read) != 0) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		delete (memory); \
		return false; \
	}

#define wrapBool(read) \
	if (!(read)) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

/**
 * @li 0x4c54414d MATL
 * @li 0x5454414d MATT
 * @li 0x4b434150 PACK
 * @li 0x41424752 RGBA
 * @li 0x5259414c LAYR
 * @li 0x4a424f72 rOBJ
 * @li 0x455a4953 SIZE
 * @li 0x4d414372 rCAM
 * @li 0x45544f4e NOTE
 * @li 0x50414d49 IMAP
 * @li 0x495a5958 XYZI
 * @li 0x5052476e nGRP
 * @li 0x4e52546e nTRN
 * @li 0x5048536e nSHP
 * @li 0x20584f56 VOX
 * @li 0x4e49414d MAIN
 */
class VoxScopedChunkWriter {
private:
	io::SeekableWriteStream& _stream;
	int64_t _chunkSizePos;
	uint32_t _chunkId;
public:
	VoxScopedChunkWriter(io::SeekableWriteStream& stream, uint32_t chunkId) : _stream(stream), _chunkId(chunkId) {
		uint8_t buf[4];
		FourCCRev(buf, chunkId);
		Log::debug("Saving %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
		stream.writeUInt32(chunkId);
		_chunkSizePos = stream.pos();
		stream.writeUInt32(0); // num bytes of chunk content
		stream.writeUInt32(0); // num bytes of children chunks
	}

	~VoxScopedChunkWriter() {
		const int64_t chunkStart = _chunkSizePos + 2 * (int64_t)sizeof(uint32_t);
		const int64_t currentPos = _stream.pos();
		core_assert_msg(chunkStart <= currentPos, "%u should be <= %u", (uint32_t)chunkStart, (uint32_t)currentPos);
		const uint64_t chunkSize = currentPos - chunkStart;
		_stream.seek(_chunkSizePos);
		_stream.writeUInt32(chunkSize); // num bytes of chunk content
		_stream.seek(currentPos);
		uint8_t buf[4];
		FourCCRev(buf, _chunkId);
		Log::debug("Chunk size for %c%c%c%c: %i", buf[0], buf[1], buf[2], buf[3], (int)chunkSize);
	}
};

class VoxScopedHeader {
private:
	io::SeekableWriteStream& _stream;
	int64_t _numBytesMainChunkPos;
	int64_t _headerSize;

public:
	VoxScopedHeader(io::SeekableWriteStream& stream) : _stream(stream) {
		stream.writeUInt32(FourCC('V','O','X',' '));
		stream.writeUInt32(150);
		stream.writeUInt32(FourCC('M','A','I','N'));
		stream.writeUInt32(0u);
		// this is filled at the end - once we know the final size of the main chunk children
		_numBytesMainChunkPos = stream.pos();
		stream.writeUInt32(0u);
		_headerSize = stream.pos();
	}

	~VoxScopedHeader() {
		// magic, version, main chunk, main chunk size, main chunk child size
		const int64_t currentPos = _stream.pos();
		const int64_t mainChildChunkSize = _stream.pos() - _headerSize;
		_stream.seek(_numBytesMainChunkPos);
		_stream.writeUInt32(mainChildChunkSize);
		_stream.seek(currentPos);
	}
};

bool VoxFormat::saveChunk_LAYR(State& state, io::SeekableWriteStream& stream, int32_t modelId, const core::String& name, bool visible) {
	VoxScopedChunkWriter scoped(stream, FourCC('L','A','Y','R'));
	wrapBool(stream.writeInt32(modelId))
	wrapBool(saveAttributes({{"_name", name}, {"_hidden", visible ? "0" : "1"}}, stream))
	wrapBool(stream.writeInt32(-1)) // must always be -1
	return true;
}

bool VoxFormat::saveChunk_nGRP(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, uint32_t childNodes) {
	VoxScopedChunkWriter scoped(stream, FourCC('n','G','R','P'));
	wrapBool(stream.writeUInt32(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.writeUInt32(childNodes))
	VoxNodeId childNodeId = nodeId + 1;
	for (uint32_t i = 0u; i < childNodes; ++i) {
		// transform and shape node pairs
		wrapBool(stream.writeUInt32(childNodeId + (i * 2u)))
	}
	return true;
}

bool VoxFormat::saveChunk_nSHP(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, int32_t modelId) {
	VoxScopedChunkWriter scoped(stream, FourCC('n','S','H','P'));
	wrapBool(stream.writeUInt32(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.writeUInt32(1u)) // shapeNodeNumModels
	wrapBool(stream.writeInt32(modelId));
	wrapBool(saveAttributes({}, stream)) // model attributes
	return true;
}

bool VoxFormat::saveChunk_nTRN(State& state, io::SeekableWriteStream& stream, VoxNodeId nodeId, VoxNodeId childNodeId, const glm::ivec3& mins, int layerId) {
	VoxScopedChunkWriter scoped(stream, FourCC('n','T','R','N'));
	wrapBool(stream.writeUInt32(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.writeUInt32(childNodeId))
	wrapBool(stream.writeInt32(-1)) // reserved - must be -1
	wrapBool(stream.writeInt32(layerId)) // layerid
	wrapBool(stream.writeUInt32(1u)) // num frames
	const core::String& translationStr = core::string::format("%i %i %i", mins.x, mins.z, mins.y);
	wrapBool(saveAttributes({{"_r", "4"}, {"_t", translationStr}}, stream))
	return true;
}

bool VoxFormat::saveChunk_SIZE(State& state, io::SeekableWriteStream& stream, const voxel::Region& region) {
	VoxScopedChunkWriter scoped(stream, FourCC('S','I','Z','E'));
	wrapBool(stream.writeUInt32(region.getWidthInVoxels()))
	wrapBool(stream.writeUInt32(region.getDepthInVoxels()))
	wrapBool(stream.writeUInt32(region.getHeightInVoxels()))
	return true;
}

bool VoxFormat::saveChunk_PACK(State& state, io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) {
	const int modelCount = (int)sceneGraph.size();
	VoxScopedChunkWriter scoped(stream, FourCC('P','A','C','K'));
	wrapBool(stream.writeUInt32(modelCount))
	return true;
}

bool VoxFormat::saveChunk_RGBA(State& state, io::SeekableWriteStream& stream) {
	const MaterialColorArray& materialColors = getMaterialColors();
	const int numColors = (int)materialColors.size();
	if (numColors > 256) {
		Log::error("More colors than supported");
		return false;
	}

	VoxScopedChunkWriter scoped(stream, FourCC('R','G','B','A'));
	for (int i = 0; i < numColors; ++i) {
		const uint32_t rgba = core::Color::getRGBA(materialColors[i]);
		wrapBool(stream.writeUInt32(rgba))
	}
	for (int i = numColors; i < 256; ++i) {
		wrapBool(stream.writeUInt32(0u))
	}
	return true;
}

bool VoxFormat::saveChunk_XYZI(State& state, io::SeekableWriteStream& stream, const voxel::RawVolume* volume, const voxel::Region& region) {
	VoxScopedChunkWriter scoped(stream, FourCC('X','Y','Z','I'));

	uint32_t numVoxels = 0;
	const uint64_t numVoxelPos = stream.pos();
	wrapBool(stream.writeUInt32(numVoxels))

	numVoxels = voxelutil::visitVolume(*volume, [&] (int x, int y, int z, const voxel::Voxel& voxel) {
		stream.writeUInt8(x - region.getLowerX());
		stream.writeUInt8(z - region.getLowerZ());
		stream.writeUInt8(y - region.getLowerY());
		const uint8_t colorIndex = voxel.getColor();
		stream.writeUInt8(colorIndex + 1);
	});
	const uint64_t chunkEndPos = stream.pos();
	wrap(stream.seek(numVoxelPos));
	wrapBool(stream.writeUInt32(numVoxels));
	wrap(stream.seek(chunkEndPos));
	return true;
}

bool VoxFormat::saveAttributes(const VoxAttributes& attributes, io::SeekableWriteStream& stream) const {
	Log::debug("Save %i attributes", (int)attributes.size());
	wrapBool(stream.writeUInt32(attributes.size()))
	for (const auto& e : attributes) {
		const core::String& key = e->first;
		const core::String& value = e->second;
		Log::debug("Save attribute %s: %s", key.c_str(), value.c_str());
		wrapBool(stream.writeUInt32(core::utf8::length(key.c_str())))
		wrapBool(stream.writeString(key, false))
		wrapBool(stream.writeUInt32(core::utf8::length(value.c_str())))
		wrapBool(stream.writeString(value, false))
	}
	return true;
}

bool VoxFormat::saveSceneGraph(State& state, io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, int modelCount) {
	const VoxNodeId rootNodeId = 0;
	VoxNodeId groupNodeId = rootNodeId + 1;
	wrapBool(saveChunk_nTRN(state, stream, rootNodeId, groupNodeId, glm::ivec3(0), -1))

	// this adds a group node with a transform+shape node pair per volume
	wrapBool(saveChunk_nGRP(state, stream, groupNodeId, modelCount))

	// the first transform node id
	VoxNodeId nodeId = groupNodeId + 1;
	int32_t modelId = 0;
	for (const SceneGraphNode& node : sceneGraph) {
		const voxel::Region &region = node.region();
		const glm::ivec3 mins = region.getCenter();
		wrapBool(saveChunk_nTRN(state, stream, nodeId, nodeId + 1, mins, 0))
		wrapBool(saveChunk_nSHP(state, stream, nodeId + 1, modelId))

		// transform + shape node per volume
		nodeId += 2;

		++modelId;
	}
	return modelCount == modelId;
}

bool VoxFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	State state;
	reset();

	SceneGraph newSceneGraph;
	splitVolumes(sceneGraph, newSceneGraph, glm::ivec3(MaxRegionSize));

	VoxScopedHeader scoped(stream);
	wrapBool(saveChunk_PACK(state, stream, newSceneGraph))
	for (const SceneGraphNode& node : newSceneGraph) {
		const voxel::Region& region = node.region();
		wrapBool(saveChunk_SIZE(state, stream, region))
		wrapBool(saveChunk_XYZI(state, stream, node.volume(), region))
	}

#if 0
	wrapBool(saveSceneGraph(state, stream, newSceneGraph, modelId))
	int modelId = 0;
	for (const SceneGraphNode& node : newSceneGraph) {
		wrapBool(saveChunk_LAYR(state, stream, modelId, node.name(), node.visible()))
		++modelId;
	}
#endif
	wrapBool(saveChunk_RGBA(state, stream))

	// IMAP
	// MATL
	// rOBJ
	// rCAM
	// NOTE
	return true;
}

bool VoxFormat::readAttributes(VoxAttributes& attributes, io::SeekableReadStream& stream) const {
	uint32_t cnt;
	wrap(stream.readUInt32(cnt))
	if (cnt > 2048) {
		Log::error("Trying to read more than the allowed number of attributes: %u", cnt);
		return false;
	}
	Log::debug("Reading %i keys in the dict", cnt);
	for (uint32_t i = 0; i < cnt; ++i) {
		char key[1024];
		char value[1024];
		uint32_t len;

		// read key
		wrap(stream.readUInt32(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(key)) {
			Log::error("Max string length for key exceeded");
			return false;
		}
		if (!stream.readString((int)len, key)) {
			Log::error("Failed to read key for dict");
			return false;
		}
		key[len] = '\0';

		// read value
		wrap(stream.readUInt32(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(value)) {
			Log::error("Max string length for value exceeded");
			return false;
		}
		if (!stream.readString((int)len, value)) {
			Log::error("Failed to read value for dict");
			return false;
		}
		value[len] = '\0';

		Log::debug("dict entry %i: %s => %s", i, key, value);
		attributes.put(key, value);
	}
	return true;
}

// 8. Default Palette : if chunk 'RGBA' is absent
// -------------------------------------------------------------------------------
static const uint32_t mvDefaultPalette[256] = {
	0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
	0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
	0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
	0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
	0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
	0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
	0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
	0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
	0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

void VoxFormat::initPalette() {
	_paletteSize = lengthof(mvDefaultPalette);
	_colorsSize = _paletteSize;
	// convert to our palette
	const MaterialColorArray& materialColors = getMaterialColors();
	for (size_t i = 0u; i < _paletteSize; ++i) {
		const uint32_t p = mvDefaultPalette[i];
		const glm::vec4& color = core::Color::fromRGBA(p);
		const int index = core::Color::getClosestMatch(color, materialColors);
		_colors[i] = p;
		_palette[i] = index;
	}
}

// 2. Chunk Structure
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 1x4      | char       | chunk id
// 4        | int        | num bytes of chunk content (N)
// 4        | int        | num bytes of children chunks (M)
// N        |            | chunk content
// M        |            | children chunks
// -------------------------------------------------------------------------------
bool VoxFormat::readChunkHeader(io::SeekableReadStream& stream, VoxChunkHeader& header) const {
	wrap(stream.readUInt32(header.chunkId))
	wrap(stream.readUInt32(header.numBytesChunk))
	wrap(stream.readUInt32(header.numBytesChildrenChunks))
	uint8_t buf[4];
	FourCCRev(buf, header.chunkId);
	Log::debug("Chunk size for %c%c%c%c: %u, childchunks: %u", buf[0], buf[1], buf[2], buf[3], header.numBytesChunk, header.numBytesChildrenChunks);
	header.nextChunkPos = stream.pos() + header.numBytesChunk + header.numBytesChildrenChunks;
	return true;
}

// 7. Chunk id 'RGBA' : palette
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4 x 256  | int        | (R, G, B, A) : 1 byte for each component
//                       | * <NOTICE>
//                       | * color [0-254] are mapped to palette index [1-255], e.g :
//                       |
//                       | for ( int i = 0; i <= 254; i++ ) {
//                       |     palette[i + 1] = ReadRGBA();
//                       | }
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_RGBA(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	const MaterialColorArray& materialColors = getMaterialColors();
	for (int i = 0; i <= 254; i++) {
		uint32_t rgba;
		wrap(stream.readUInt32(rgba))
		_colors[i] = rgba;
		const glm::vec4& color = core::Color::fromRGBA(rgba);
		const int index = core::Color::getClosestMatch(color, materialColors);
		Log::trace("rgba %x, r: %f, g: %f, b: %f, a: %f, index: %i, r2: %f, g2: %f, b2: %f, a2: %f",
				rgba, color.r, color.g, color.b, color.a, index, materialColors[index].r, materialColors[index].g, materialColors[index].b, materialColors[index].a);
		_palette[i + 1] = (uint8_t)index;
	}
	_paletteSize = 255;
	_colorsSize = 255;
	return true;
}

// 5. Chunk id 'SIZE' : model size
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4        | int        | size x
// 4        | int        | size y
// 4        | int        | size z : gravity direction
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_SIZE(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	// we have to flip the axis here
	uint32_t x, y, z;
	wrap(stream.readUInt32(x))
	wrap(stream.readUInt32(z))
	wrap(stream.readUInt32(y))
	glm::ivec3 maxsregion((int32_t)(x) - 1, (int32_t)(y) - 1, (int32_t)(z) - 1);
	Log::debug("Found size chunk: (%u:%u:%u)", x, y, z);
	Region region(glm::ivec3(0), maxsregion);
	if (!region.isValid()) {
		Log::error("Found invalid region in vox file: %i:%i:%i", x, y, z);
		return false;
	}
	if (region.getWidthInVoxels() > MaxRegionSize || region.getHeightInVoxels() > MaxRegionSize ||
		region.getDepthInVoxels() > MaxRegionSize) {
		Log::error("Found invalid region in vox file: %i:%i:%i", x, y, z);
		return false;
	}
	state._sizes.push_back(region);
	if (state._sizes.size() > 256) {
		Log::error("Found more than 256 models: %i", (int)state._sizes.size());
		return false;
	}
	return true;
}

glm::ivec3 VoxFormat::calcTransform(State& state, const VoxNTRNNode& t, const glm::ivec3& pos, const glm::vec3& pivot) const {
	const glm::ivec3 rotated(glm::floor(t.rotMat * (glm::vec3(pos) - pivot)));
	return rotated + t.translation;
}

glm::ivec3 VoxFormat::calcTransform(State& state, const VoxNTRNNode& t, int x, int y, int z, const glm::vec3& pivot) const {
	return calcTransform(state, t, glm::ivec3(x, y, z), pivot);
}

// 6. Chunk id 'XYZI' : model voxels
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4        | int        | numVoxels (N)
// 4 x N    | int        | (x, y, z, colorIndex) : 1 byte for each component
//                       | Note: Z axis is vertical axis, so you might want to
//                       | read them first and then assemble as (x, z, y) for Y-up
//                       | coordinate system.
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_XYZI(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	uint32_t numVoxels;
	wrap(stream.readUInt32(numVoxels))
	Log::debug("Found voxel chunk with %u voxels", numVoxels);
	const int modelId = (int)state._xyzi.size();
	if (state._sizes.empty() || modelId >= (int)state._sizes.size()) {
		Log::error("Invalid XYZI chunk without previous SIZE chunk");
		return false;
	}

	const voxel::Region& region = state._sizes[modelId];
	const glm::vec3 size(region.getDimensionsInVoxels());
	const glm::vec3 pivot(size / 2.0f - 0.5f);
	const VoxNTRNNode& transform = traverseTransform(state, modelId);
	const glm::ivec3& transformedMins = calcTransform(state, transform, region.getLowerCorner(), pivot);
	const glm::ivec3& transformedMaxs = calcTransform(state, transform, region.getUpperCorner(), pivot);
	const glm::ivec3 mins = glm::min(transformedMins, transformedMaxs);
	const glm::ivec3 maxs = glm::max(transformedMins, transformedMaxs);
	RawVolume *volume = new RawVolume({mins, maxs});
	int volumeVoxelSet = 0;
	for (uint32_t i = 0; i < numVoxels; ++i) {
		uint8_t x, y, z, colorIndex;
		wrapFree(stream.readUInt8(x), volume)
		wrapFree(stream.readUInt8(z), volume)
		wrapFree(stream.readUInt8(y), volume)
		wrapFree(stream.readUInt8(colorIndex), volume)
		const uint8_t index = convertPaletteIndex(colorIndex);
		voxel::VoxelType voxelType = voxel::VoxelType::Generic;
		const voxel::Voxel& voxel = voxel::createVoxel(voxelType, index);
		const glm::ivec3& pos = calcTransform(state, transform, x, y, z, pivot);
		if (volume->setVoxel(pos, voxel)) {
			++volumeVoxelSet;
		}
	}
	Log::debug("Loaded layer %i with %i voxels (%i)", modelId, numVoxels, volumeVoxelSet);
	VoxModel model;
	model.volume = volume;
	state._xyzi.push_back(model);
	return true;
}

bool VoxFormat::loadChunk_nSHP(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxNSHPNode nshp;
	wrap(stream.readUInt32(nshp.nodeId))
	Log::debug("shape node: %u", nshp.nodeId);
	wrapBool(readAttributes(nshp.attributes, stream))
	wrap(stream.readUInt32(nshp.shapeNodeNumModels))
	if (nshp.shapeNodeNumModels != 1) {
		Log::error("Shape node chunk contained a numModels value != 1: %i", nshp.shapeNodeNumModels);
		return false;
	}
	wrap(stream.readInt32(nshp.modelId))
	wrapBool(readAttributes(nshp.modelAttributes, stream))
	state._nshp.put(nshp.nodeId, nshp);
	const VoxSceneGraphNode sceneNode{nshp.nodeId, VoxSceneGraphNodeType::Shape, VoxSceneGraphChildNodes(0)};
	state._sceneGraph.put(nshp.nodeId, sceneNode);
	return true;
}

// 4. Chunk id 'PACK' : if it is absent, only one model in the file
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4        | int        | numModels : num of SIZE and XYZI chunks
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_PACK(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	wrap(stream.readUInt32(state._numPacks))
	return true;
}

// 9. Chunk id 'MATT' : material, if it is absent, it is diffuse material
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4        | int        | id [1-255]
//
// 4        | int        | material type
//                       | 0 : diffuse
//                       | 1 : metal
//                       | 2 : glass
//                       | 3 : emissive
//
// 4        | float      | material weight
//                       | diffuse  : 1.0
//                       | metal    : (0.0 - 1.0] : blend between metal and diffuse material
//                       | glass    : (0.0 - 1.0] : blend between glass and diffuse material
//                       | emissive : (0.0 - 1.0] : self-illuminated material
//
// 4        | int        | property bits : set if value is saved in next section
//                       | bit(0) : Plastic
//                       | bit(1) : Roughness
//                       | bit(2) : Specular
//                       | bit(3) : IOR
//                       | bit(4) : Attenuation
//                       | bit(5) : Power
//                       | bit(6) : Glow
//                       | bit(7) : isTotalPower (*no value)
//
// 4 * N    | float      | normalized property value : (0.0 - 1.0]
//                       | * need to map to real range
//                       | * Plastic material only accepts {0.0, 1.0} for this version
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_MATT(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	// TODO: this is deprecated - MATL is the v2 version
	uint32_t materialId;
	wrap(stream.readUInt32(materialId))
	uint32_t materialType;
	wrap(stream.readUInt32(materialType))
	float materialWeight;
	wrap(stream.readFloat(materialWeight))
	uint32_t materialProperties;
	wrap(stream.readUInt32(materialProperties))
#if 0
	for (uint32_t i = 0; i < numBytesChunk; ++i) {
		float materialPropertyValue;
		wrap(stream.readFloat(materialPropertyValue))
	}
#endif
	return true;
}

// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
bool VoxFormat::loadChunk_LAYR(State& state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxLayer layer;
	wrap(stream.readUInt32(layer.layerId))
	// (_name : string)
	// (_hidden : 0/1)
	wrapBool(readAttributes(layer.attributes, stream))
	int32_t end;
	wrap(stream.readInt32(end));
	if (end != -1) {
		Log::error("Unexpected end of LAYR chunk - expected -1, got %i", (int)end);
		return true;
	}
	state._layers.push_back(layer);
	return true;
}

// Note: in order to build transformations, you need to traverse the scene
// graph recursively, starting with root Transform node, and then for every
// next transform:
//
// * Translations are added when traversing
// * Rotation matrices are multiplied (parent * transform)
//
// It's preferable to maintain two separate stacks for translations
// (vec3 stack), and a matrix stack. For every Transfrom node, you peek the
// stacks, and add vector in a stack to Transform node's translation vector,
// and multiply stack's rotation matrix by Transform node's rotation matrix,
// and push these values to their respective stacks.
//
// For every Shape node, you can just pop both values and use them.
bool VoxFormat::parseSceneGraphTranslation(VoxNTRNNode& transform, const VoxAttributes& attributes) const {
	auto trans = attributes.find("_t");
	if (trans == attributes.end()) {
		return true;
	}
	const core::String& translations = trans->second;
	glm::ivec3& v = transform.translation;
	if (SDL_sscanf(translations.c_str(), "%d %d %d", &v.x, &v.z, &v.y) != 3) {
		Log::error("Failed to parse translation");
		return false;
	}
	Log::debug("translation %i:%i:%i", v.x, v.y, v.z);
	return true;
}

// ROTATION type
//
// store a row-major (XZY) rotation in the bits of a byte
//
// for example :
// R =
//  0  1  0
//  0  0 -1
// -1  0  0
// ==>
// unsigned char _r = (1 << 0) | (2 << 2) | (0 << 4) | (1 << 5) | (1 << 6)
//
// bit | value
// 0-1 : 1 : index of the non-zero entry in the first row
// 2-3 : 2 : index of the non-zero entry in the second row
// 4   : 0 : the sign in the first row (0 : positive; 1 : negative)
// 5   : 1 : the sign in the second row (0 : positive; 1 : negative)
// 6   : 1 : the sign in the third row (0 : positive; 1 : negative)
bool VoxFormat::parseSceneGraphRotation(VoxNTRNNode &transform, const VoxAttributes &attributes) const {
	auto rot = attributes.find("_r");
	if (rot == attributes.end()) {
		return true;
	}

	const uint8_t packed = core::string::toInt(rot->second);
	const VoxRotationMatrixPacked *packedRot = (const VoxRotationMatrixPacked *)&packed;
	const uint8_t nonZeroEntryInThirdRow = 3u - (packedRot->nonZeroEntryInFirstRow + packedRot->nonZeroEntryInSecondRow);

	// sanity checks
	if (packedRot->nonZeroEntryInFirstRow == packedRot->nonZeroEntryInSecondRow) {
		Log::error("Rotation column indices invalid");
		return false;
	}
	if (packedRot->nonZeroEntryInFirstRow > 2 || packedRot->nonZeroEntryInSecondRow > 2 || nonZeroEntryInThirdRow > 2) {
		Log::error("Rotation column indices out of bounds");
		return false;
	}

	// glm is column major - thus we have to flip the col/row indices here
	transform.rotMat[packedRot->nonZeroEntryInFirstRow][0] = packedRot->signInFirstRow ? -1.0f : 1.0f;
	transform.rotMat[packedRot->nonZeroEntryInSecondRow][1] = packedRot->signInThirdRow ? -1.0f : 1.0f;
	transform.rotMat[nonZeroEntryInThirdRow][2] = packedRot->signInSecondRow ? -1.0f : 1.0f;

	for (int i = 0; i < 3; ++i) {
		Log::debug("mat3[%i]: %.2f, %.2f, %.2f", i, transform.rotMat[0][i], transform.rotMat[1][i], transform.rotMat[2][i]);
	}

	return true;
}

// (_r : int8) ROTATION in STRING (i.e. "36")
// (_t : int32x3) translation in STRING format separated by spaces (i.e. "-1 10 4"). The anchor for these translations is center of the box.
bool VoxFormat::loadChunk_nTRN(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxNTRNNode transform;
	uint32_t nodeId;
	wrap(stream.readUInt32(nodeId))
	if (state._sceneGraph.empty()) {
		state._rootNode = nodeId;
	}
	Log::debug("transform node: %u", nodeId);
	wrapBool(readAttributes(transform.attributes, stream))
	wrap(stream.readUInt32(transform.childNodeId))
	wrap(stream.readUInt32(transform.reserved))
	wrap(stream.readInt32(transform.layerId))
	wrap(stream.readUInt32(transform.numFrames))
	Log::debug("nTRN chunk: node: %u, childNodeId: %u, layerId: %i, numFrames: %u",
		nodeId, transform.childNodeId, transform.layerId, transform.numFrames);
	if (transform.numFrames != 1) {
		Log::warn("Transform node chunk contains an expected value for numFrames: %i", transform.numFrames);
	}
	for (uint32_t f = 0; f < transform.numFrames; ++f) {
		wrapBool(readAttributes(transform.transformNodeAttributes, stream))
	}

	wrapBool(parseSceneGraphRotation(transform, transform.transformNodeAttributes))
	wrapBool(parseSceneGraphTranslation(transform, transform.transformNodeAttributes))

	VoxSceneGraphChildNodes child(1);
	child[0] = transform.childNodeId;
	const VoxSceneGraphNode sceneNode{nodeId, VoxSceneGraphNodeType::Transform, child};
	Log::debug("transform child node id: %u, nodeId: %u", sceneNode.childNodeIds[0], nodeId);
	state._sceneGraph.put(nodeId, sceneNode);
	state._ntrn.put(nodeId, transform);
	state._parentNodes.put(transform.childNodeId, nodeId);

	return true;
}

bool VoxFormat::loadChunk_nGRP(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxNGRPNode ngrp;
	uint32_t nodeId;
	wrap(stream.readUInt32(nodeId))
	Log::debug("group node: %u", nodeId);
	wrapBool(readAttributes(ngrp.attributes, stream))
	uint32_t numChildren;
	wrap(stream.readUInt32(numChildren))

	ngrp.children.reserve(numChildren);
	for (uint32_t i = 0; i < numChildren; ++i) {
		uint32_t child;
		wrap(stream.readUInt32(child))
		ngrp.children.push_back((child));
		state._parentNodes.put(child, nodeId);
	}
	const VoxSceneGraphNode sceneNode{nodeId, VoxSceneGraphNodeType::Group, ngrp.children};
	state._sceneGraph.put(nodeId, sceneNode);
	state._ngrp.put(nodeId, ngrp);
	return true;
}

bool VoxFormat::loadChunk_rCAM(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxCamera camera;
	wrap(stream.readUInt32(camera.cameraId))
	// (_mode : string - pers)
	// (_focus : vec(3))
	// (_angle : vec(3))
	// (_radius : int)
	// (_frustum : float)
	// (_fov : int degree)
	wrapBool(readAttributes(camera.attributes, stream))
	state._cameras.push_back(camera);
	return true;
}

// the rendering setting are not open yet because they are changing frequently.
// But you can still read it since it is just in the DICT format.
bool VoxFormat::loadChunk_rOBJ(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	VoxROBJ rOBJ;
	wrapBool(readAttributes(rOBJ.attributes, stream))
	// _type => _setting
	// _ground => 1
	// _grid => 0
	// _edge => 0
	// _bg_c => 0
	// _bg_a => 0
	// _scale => 1 1 1
	// _cell => 1

	// _type => _bloom
	// _mix => 0.11
	// _scale => 0.27
	// _aspect => 0
	// _threshold => 1

	// _type => _edge
	// _color => 0 0 0
	// _width => 0.2

	// _type => _bg
	// _color => 0 0 0

	// _type => _bounce
	// _diffuse => 2
	// _specular => 5
	// _scatter => 5
	// _energy => 3

	// _type => _env
	// _mode => 0

	// _type => _inf
	// _i => 0.55
	// _k => 255 255 255
	// _angle => 40 334
	// _area => 0
	// _disk => 0

	// _type => _uni
	// _i => 0.58
	// _k => 215 255 253

	// _type => _ibl
	// _path => HDR_041_Path_Env.hdr
	// _i => 1
	// _rot => 0

	// _type => _atm
	// _ray_d => 0.4
	// _ray_k => 45 104 255
	// _mie_d => 0.4
	// _mie_k => 255 255 255
	// _mie_g => 0.85
	// _o3_d => 0
	// _o3_k => 105 255 110

	// _type => _fog_uni
	// _d => 0
	// _k => 255 255 255
	// _g => 0

	// _type => _lens
	// _proj => 0
	// _fov => 45
	// _aperture => 0.25
	// _blade_n => 0
	// _blade_r => 0

	// _type => _film
	// _expo => 1
	// _vig => 0
	// _aces => 1
	// _gam => 2.2

	state._robjs.push_back(rOBJ);
	return true;
}

bool VoxFormat::loadChunk_MATL(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	uint32_t materialId;
	wrap(stream.readUInt32(materialId))
	VoxAttributes materialAttributes;
	// (_type : str) _diffuse, _metal, _glass, _emit
	// (_weight : float) range 0 ~ 1
	// (_rough : float)
	// (_spec : float)
	// (_ior : float)
	// (_att : float)
	// (_d : float)
	// (_flux : float)
	// (_emit : float)
	// (_plastic : 0|1)
	// (_metal : 0|1)
	wrapBool(readAttributes(materialAttributes, stream))
	return true;
}

// Represents the palette "index" map
// TODO: take this into account while mapping the colors - see _palette member
bool VoxFormat::loadChunk_IMAP(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	for (int i = 0; i < 256; i++) {
		uint8_t paletteIndex;
		wrap(stream.readUInt8(paletteIndex))
	}
	return true;
}

// Contains all color type names
bool VoxFormat::loadChunk_NOTE(State &state, io::SeekableReadStream& stream, const VoxChunkHeader& header) {
	uint32_t numColorNames;
	wrap(stream.readUInt32(numColorNames))
	for (uint32_t i = 0; i < numColorNames; ++i) {
		uint32_t len;
		char name[1024];
		wrap(stream.readUInt32(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(name)) {
			Log::error("Max string length for color name exceeded");
			return false;
		}
		if (!stream.readString((int)len, name)) {
			Log::error("Failed to read color name");
			return false;
		}
		name[len] = '\0';
		Log::debug("Found color name %s", name);
	}
	return true;
}

bool VoxFormat::loadFirstChunks(State &state, io::SeekableReadStream& stream) {
	do {
		VoxChunkHeader header;
		wrapBool(readChunkHeader(stream, header))
		switch (header.chunkId) {
		case FourCC('M','A','T','L'):
			wrapBool(loadChunk_MATL(state, stream, header))
			break;
		case FourCC('M','A','T','T'):
			wrapBool(loadChunk_MATT(state, stream, header))
			break;
		case FourCC('P','A','C','K'):
			wrapBool(loadChunk_PACK(state, stream, header))
			break;
		case FourCC('R','G','B','A'):
			wrapBool(loadChunk_RGBA(state, stream, header))
			break;
		case FourCC('I','M','A','P'):
			wrapBool(loadChunk_IMAP(state, stream, header))
			break;
		case FourCC('N','O','T','E'):
			wrapBool(loadChunk_NOTE(state, stream, header))
			break;
		case FourCC('r','C','A','M'):
			wrapBool(loadChunk_rCAM(state, stream, header))
			break;
		case FourCC('r','O','B','J'):
			wrapBool(loadChunk_rOBJ(state, stream, header))
			break;
		case FourCC('S','I','Z','E'):
			wrapBool(loadChunk_SIZE(state, stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos))
	} while (stream.remaining() > 0);

	return true;
}

bool VoxFormat::loadSecondChunks(State &state, io::SeekableReadStream& stream) {
	do {
		VoxChunkHeader header;
		wrapBool(readChunkHeader(stream, header));
		switch (header.chunkId) {
		case FourCC('L','A','Y','R'):
			wrapBool(loadChunk_LAYR(state, stream, header))
			break;
		case FourCC('X','Y','Z','I'):
			wrapBool(loadChunk_XYZI(state, stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos));
	} while (stream.remaining() > 0);
	return true;
}

// Scene Graph
//
// T : Transform Node
// G : Group Node
// S : Shape Node
//
//     T    //
//     |    //
//     G    //
//    / \   //
//   T   T  //
//   |   |  //
//   G   S  //
//  / \     //
// T   T    //
// |   |    //
// S   S    //
bool VoxFormat::loadSceneGraph(State &state, io::SeekableReadStream& stream) {
	do {
		VoxChunkHeader header;
		wrapBool(readChunkHeader(stream, header))

		switch (header.chunkId) {
		case FourCC('n','G','R','P'):
			state._foundSceneGraph = true;
			wrapBool(loadChunk_nGRP(state, stream, header))
			break;
		case FourCC('n','T','R','N'):
			state._foundSceneGraph = true;
			wrapBool(loadChunk_nTRN(state, stream, header))
			break;
		case FourCC('n','S','H','P'):
			state._foundSceneGraph = true;
			wrapBool(loadChunk_nSHP(state, stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos));
	} while (stream.remaining() > 0);

	return true;
}

VoxFormat::VoxNTRNNode VoxFormat::traverseTransform(State &state, int32_t modelId) const {
	VoxNTRNNode transform;
	for (const auto& entry : state._nshp) {
		const VoxNSHPNode &nshp = entry->value;
		if (nshp.modelId == modelId) {
			concatTransform(state, transform, nshp.nodeId);
			break;
		}
	}
	return transform;
}

bool VoxFormat::concatTransform(State &state, VoxNTRNNode& transform, VoxNodeId nodeId) const {
	VoxNodeId parent;
	VoxNodeId current = nodeId;
	while (state._parentNodes.get(current, parent)) {
		wrapBool(concatTransform(state, transform, parent))
		current = parent;
	}

	VoxSceneGraphNode node;
	if (!state._sceneGraph.get(nodeId, node)) {
		Log::debug("Could not find node %u", nodeId);
		return false;
	}

	if (node.type == VoxSceneGraphNodeType::Transform) {
		auto ntrniter = state._ntrn.find(node.nodeId);
		if (ntrniter == state._ntrn.end()) {
			Log::error("Invalid transform node id found: %u", node.nodeId);
			return false;
		}

		const VoxNTRNNode& t = ntrniter->value;
		transform.rotMat = t.rotMat * transform.rotMat;
		transform.translation += t.rotMat * glm::vec3(t.translation);
		Log::debug("Apply translation for node %u %i:%i:%i",
				nodeId, transform.translation.x, transform.translation.y, transform.translation.z);
	}

	return true;
}

bool VoxFormat::checkVersionAndMagic(io::SeekableReadStream& stream) const {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('V','O','X',' ')) {
		Log::error("Could not load vox file: Invalid magic found");
		return false;
	}

	uint32_t version;
	wrap(stream.readUInt32(version))

	if (version != 150) {
		Log::warn("Vox file loading is only tested for version 150 - but we've found %i", version);
	}
	return true;
}

bool VoxFormat::checkMainChunk(io::SeekableReadStream& stream) const {
	VoxChunkHeader main;
	wrapBool(readChunkHeader(stream, main))
	// 3. Chunk id 'MAIN' : the root chunk and parent chunk of all the other chunks
	if (main.chunkId != FourCC('M','A','I','N')) {
		Log::error("Could not load vox file: Invalid magic for main chunk found");
		return false;
	}

	if (stream.remaining() < main.numBytesChildrenChunks) {
		Log::error("Could not load vox file: Incomplete file");
		return false;
	}

	return true;
}

bool VoxFormat::fillSceneGraph_r(State& state, VoxNodeId nodeId, voxel::SceneGraph& sceneGraph, int parentId) {
	auto iter = state._sceneGraph.find(nodeId);
	if (iter == state._sceneGraph.end()) {
		Log::error("Can't find scene graph node %u in mv scene graph", nodeId);
		return false;
	}
	const VoxSceneGraphNode& voxSceneGraphNode = iter->value;
	voxel::SceneGraphNodeType type = voxel::SceneGraphNodeType::Max;
	switch (voxSceneGraphNode.type) {
	case VoxSceneGraphNodeType::Transform:
		type = voxel::SceneGraphNodeType::Transform;
		break;
	case VoxSceneGraphNodeType::Group:
		type = voxel::SceneGraphNodeType::Group;
		break;
	case VoxSceneGraphNodeType::Shape:
		type = voxel::SceneGraphNodeType::ModelReference;
		break;
	}
	if (type == voxel::SceneGraphNodeType::Max) {
		Log::error("Can't map mv scene graph node type %i to SceneGraphNodeType", (int)voxSceneGraphNode.type);
		return false;
	}
	voxel::SceneGraphNode node(type);
	Log::debug("node with parent %i", parentId);

	switch (voxSceneGraphNode.type) {
	case VoxSceneGraphNodeType::Transform: {
		auto niter = state._ntrn.find(voxSceneGraphNode.nodeId);
		if (niter == state._ntrn.end()) {
			Log::error("Can't find nTRN node with id %u", voxSceneGraphNode.nodeId);
			return false;
		}
		const VoxNTRNNode& voxnode = niter->value;
		node.addProperties(voxnode.attributes);
		node.addProperties(voxnode.transformNodeAttributes);
		node.setProperty("layer", core::string::toString(voxnode.layerId));
		node.setName("Transform");
		break;
	}
	case VoxSceneGraphNodeType::Shape: {
		auto niter = state._nshp.find(voxSceneGraphNode.nodeId);
		if (niter == state._nshp.end()) {
			Log::error("Can't find nSHP node with id %u", voxSceneGraphNode.nodeId);
			return false;
		}
		const VoxNSHPNode& voxnode = niter->value;
		node.addProperties(voxnode.attributes);
		node.addProperties(voxnode.modelAttributes);
		node.setName("Model");
		if (voxnode.modelId >= (int32_t)state._xyzi.size()) {
			Log::error("Invalid model id given in nSHP node: %u", voxnode.modelId);
			return false;
		}
		voxel::SceneGraphNode *node = sceneGraph[(int)voxnode.modelId];
		if (node == nullptr) {
			Log::error("Failed to resolve model node with model id %i", (int)voxnode.modelId);
			return false;
		}
		const int modelNodeId = node->id();
		Log::debug("Model %i is scene graph node %i", (int)voxnode.modelId, modelNodeId);
		node->setReferencedNodeId(modelNodeId);
		// TODO: node.setMatrix();
		break;
	}
	case VoxSceneGraphNodeType::Group: {
		auto niter = state._ngrp.find(voxSceneGraphNode.nodeId);
		if (niter == state._ngrp.end()) {
			Log::error("Can't find nGRP node with id %u", voxSceneGraphNode.nodeId);
			return false;
		}
		const VoxNGRPNode& voxnode = niter->value;
		node.addProperties(voxnode.attributes);
		node.setName("Group");
		break;
	}
	}

	const int newNodeId = sceneGraph.emplace(core::move(node), parentId);
	for (VoxNodeId childNodeId : voxSceneGraphNode.childNodeIds) {
		if (!fillSceneGraph_r(state, childNodeId, sceneGraph, newNodeId)) {
			return false;
		}
	}
	return true;
}

// 1. File Structure : RIFF style
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 1x4      | char       | id 'VOX ' : 'V' 'O' 'X' 'space', 'V' is first
// 4        | int        | version number : 150
//
// Chunk 'MAIN'
// {
//     // pack of models
//     Chunk 'PACK'    : optional
//
//     // models
//     Chunk 'SIZE'
//     Chunk 'XYZI'
//
//     Chunk 'SIZE'
//     Chunk 'XYZI'
//
//     ...
//
//     Chunk 'SIZE'
//     Chunk 'XYZI'
//
//     // palette
//     Chunk 'RGBA'    : optional
//
//     // materials
//     Chunk 'MATT'    : optional
//     Chunk 'MATT'
//     ...
//     Chunk 'MATT'
// }
// -------------------------------------------------------------------------------
bool VoxFormat::loadGroups(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	reset();

	State state;
	wrapBool(checkVersionAndMagic(stream))
	wrapBool(checkMainChunk(stream))

	const int64_t resetPos = stream.pos();
	wrapBool(loadFirstChunks(state, stream))
	stream.seek(resetPos);

	wrapBool(loadSceneGraph(state, stream))
	stream.seek(resetPos);

	wrapBool(loadSecondChunks(state, stream))

	const int sceneGraphRootNodeId = sceneGraph.root().id();
	// add the models as the first nodes!
	for (VoxModel &e : state._xyzi) {
		voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Model);
		core_assert(e.volume);
		node.setVolume(e.volume, true);
		const int modelNodeId = sceneGraph.emplace(core::move(node), sceneGraphRootNodeId);
		Log::debug("Model node id: %i", modelNodeId);
		e.volume = nullptr;
	}

	if (state._foundSceneGraph) {
		if (!fillSceneGraph_r(state, state._rootNode, sceneGraph, sceneGraphRootNodeId)) {
			Log::warn("Could not load the scene graph");
		}
	}

	if (!state._cameras.empty()) {
		voxel::SceneGraphNode groupNode(voxel::SceneGraphNodeType::Group);
		groupNode.setName("Cameras");
		const int groupNodeId = sceneGraph.emplace(core::move(groupNode), sceneGraphRootNodeId);
		for (const VoxCamera &e : state._cameras) {
			voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Camera);
			node.addProperties(e.attributes);
			sceneGraph.emplace(core::move(node), groupNodeId);
		}
	}

	if (!state._robjs.empty()) {
		voxel::SceneGraphNode groupNode(voxel::SceneGraphNodeType::Group);
		groupNode.setName("Render settings");
		const int groupNodeId = sceneGraph.emplace(core::move(groupNode), sceneGraphRootNodeId);
		for (const VoxROBJ &e : state._robjs) {
			voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Unknown);
			node.addProperties(e.attributes);
			sceneGraph.emplace(core::move(node), groupNodeId);
		}
	}

	if (!state._layers.empty()) {
		const uint32_t modelCount = sceneGraph.size(voxel::SceneGraphNodeType::Model);
		voxel::SceneGraphNode groupNode(voxel::SceneGraphNodeType::Group);
		groupNode.setName("Layers");
		const int groupNodeId = sceneGraph.emplace(core::move(groupNode), sceneGraphRootNodeId);
		for (const VoxLayer &layer : state._layers) {
			core::String layerName;
			layer.attributes.get("_name", layerName);
			voxel::SceneGraphNode layerNode(voxel::SceneGraphNodeType::Unknown);
			layerNode.addProperties(layer.attributes);
			layerNode.setName(layerName);
			if (layer.modelIdx <= modelCount) {
				core::String property;
				layer.attributes.get("_hidden", property);
				voxel::SceneGraphNode *node = sceneGraph[(int)layer.modelIdx];
				if (node == nullptr) {
					Log::error("Failed to resolve model node with model id %i", (int)layer.modelIdx);
					return false;
				}
				node->setVisible(property.empty() || property == "0");
			} else {
				Log::warn("Invalid layer model id: %i (model count: %i)", layer.modelIdx, modelCount);
			}
			sceneGraph.emplace(core::move(layerNode), groupNodeId);
		}
	}

	for (const auto& entry : state._nshp) {
		const VoxNSHPNode& nshp = entry->value;
		SceneGraphNode* modelNode = sceneGraph[nshp.modelId];
		if (modelNode == nullptr) {
			Log::error("Failed to resolve model node with model id %i", nshp.modelId);
			return false;
		}
		if (!modelNode->name().empty()) {
			continue;
		}
		VoxNodeId nodeId = nshp.nodeId;
		for (;;) {
			auto i = state._parentNodes.find(nodeId);
			if (i == state._parentNodes.end()) {
				break;
			}
			nodeId = i->second;
			auto nodeIter = state._sceneGraph.find(nodeId);
			if (nodeIter == state._sceneGraph.end()) {
				Log::error("Node with id %i wasn't found in the graph", nodeId);
				continue;
			}
			const VoxSceneGraphNode& voxnode = nodeIter->second;
			switch (voxnode.type) {
			case VoxSceneGraphNodeType::Transform: {
				auto nameIter = state._ntrn.find(voxnode.nodeId)->second.attributes.find("_name");
				if (nameIter != nshp.attributes.end()) {
					modelNode->setName(nameIter->value);
				}
				break;
			}
			case VoxSceneGraphNodeType::Shape: {
				auto nameIter = state._nshp.find(voxnode.nodeId)->second.attributes.find("_name");
				if (nameIter != nshp.attributes.end()) {
					modelNode->setName(nameIter->value);
				}
				break;
			}
			case VoxSceneGraphNodeType::Group:
				break;
			}
			if (!modelNode->name().empty()) {
				break;
			}
		}
	}

	return !sceneGraph.empty(voxel::SceneGraphNodeType::Model);
}

size_t VoxFormat::loadPalette(const core::String& filename, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) {
	reset();
	for (int i = 0; i < lengthof(mvDefaultPalette); ++i) {
		_colors[i] = mvDefaultPalette[i];
	}
	wrapBool(checkVersionAndMagic(stream))
	wrapBool(checkMainChunk(stream))

	State state;
	do {
		VoxChunkHeader header;
		wrapBool(readChunkHeader(stream, header))
		switch (header.chunkId) {
		case FourCC('R','G','B','A'):
			wrapBool(loadChunk_RGBA(state, stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos))
	} while (stream.remaining() > 0);
	palette = _colors;
	return _colorsSize;
}

void VoxFormat::reset() {
	initPalette();
}

#undef wrap
#undef wrapBool
#undef wrapFree

}
