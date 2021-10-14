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
#include "voxel/MaterialColor.h"
#include "voxelutil/VolumeVisitor.h"
#include <SDL_assert.h>
#include <glm/gtc/matrix_access.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

#define wrapBool(read) \
	if (!(read)) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

class ScopedChunkWriter {
private:
	io::FileStream& _stream;
	uint64_t _chunkSizePos;
	uint32_t _chunkId;
public:
	ScopedChunkWriter(io::FileStream& stream, uint32_t chunkId) : _stream(stream), _chunkId(chunkId) {
		uint8_t buf[4];
		FourCCRev(buf, chunkId);
		Log::debug("Saving %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
		stream.addInt(chunkId);
		_chunkSizePos = stream.pos();
		stream.addInt(0);
		stream.addInt(0);
	}

	~ScopedChunkWriter() {
		const uint64_t chunkStart = _chunkSizePos + 2 * sizeof(uint32_t);
		const uint64_t currentPos = _stream.pos();
		core_assert_msg(chunkStart <= currentPos, "%u should be <= %u", (uint32_t)chunkStart, (uint32_t)currentPos);
		const uint64_t chunkSize = currentPos - chunkStart;
		_stream.seek(_chunkSizePos);
		_stream.addInt(chunkSize);
		_stream.seek(currentPos);
		uint8_t buf[4];
		FourCCRev(buf, _chunkId);
		Log::debug("Chunk size for %c%c%c%c: %i", buf[0], buf[1], buf[2], buf[3], (int)chunkSize);
	}
};

class ScopedHeader {
private:
	io::FileStream& _stream;
	int64_t _chunkCountPos;
	int64_t _numBytesMainChunkPos;
	int64_t _headerSize;
	uint32_t &_chunks;

public:
	ScopedHeader(io::FileStream& stream, uint32_t &chunks) : _stream(stream), _chunks(chunks) {
		stream.addInt(FourCC('V','O','X',' '));
		stream.addInt(150);
		stream.addInt(FourCC('M','A','I','N'));
		_chunkCountPos = stream.pos();
		stream.addInt(0);
		// this is filled at the end - once we know the final size of the main chunk children
		_numBytesMainChunkPos = stream.pos();
		stream.addInt(0);
		_headerSize = stream.pos();
	}

	~ScopedHeader() {
		// magic, version, main chunk, main chunk size, main chunk child size
		const uint64_t currentPos = _stream.pos();
		const int64_t mainChildChunkSize = _stream.pos() - _headerSize;
		_stream.seek(_chunkCountPos);
		_stream.addInt(_chunks);
		_stream.seek(_numBytesMainChunkPos);
		_stream.addInt(mainChildChunkSize);
		_stream.seek(currentPos);
		Log::debug("Chunks in main: %u", _chunks);
	}
};

bool VoxFormat::saveChunk_LAYR(io::FileStream& stream, int modelId, const core::String& name, bool visible) {
	ScopedChunkWriter scoped(stream, FourCC('L','A','Y','R'));
	wrapBool(stream.addInt(modelId))
	const core::String attributeName = "_name";
	const core::String attributeVisible = "_visible";
	wrapBool(saveAttributes({{attributeName, name}, {attributeVisible, visible ? "1" : "0"}}, stream))
	wrapBool(stream.addInt((uint32_t)-1)) // must always be -1
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_nGRP(io::FileStream& stream, NodeId nodeId, uint32_t volumes) {
	ScopedChunkWriter scoped(stream, FourCC('n','G','R','P'));
	wrapBool(stream.addInt(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.addInt(volumes))
	NodeId childNodeId = nodeId + 1;
	for (uint32_t i = 0u; i < volumes; ++i) {
		// transform and shape node pairs
		wrapBool(stream.addInt(childNodeId + (i * 2u)))
	}
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_nSHP(io::FileStream& stream, NodeId nodeId, uint32_t volumeId) {
	ScopedChunkWriter scoped(stream, FourCC('n','S','H','P'));
	wrapBool(stream.addInt(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.addInt(1)) // shapeNodeNumModels
	wrapBool(stream.addInt(volumeId));
	wrapBool(saveAttributes({}, stream)) // model attributes
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_nTRN(io::FileStream& stream, NodeId nodeId, NodeId childNodeId, const glm::ivec3& mins) {
	ScopedChunkWriter scoped(stream, FourCC('n','T','R','N'));
	wrapBool(stream.addInt(nodeId))
	wrapBool(saveAttributes({}, stream))
	wrapBool(stream.addInt(childNodeId))
	wrapBool(stream.addInt(-1)) // reserved - must be -1
	wrapBool(stream.addInt(0)) // layerid ???
	wrapBool(stream.addInt(1)) // num frames
	if (mins.x != 0 || mins.y != 0 || mins.z != 0) {
		constexpr glm::mat3 rot(1.0f);
		const glm::vec3& newMins = rot * glm::vec3(mins);
		const core::String& translationStr = core::string::format("%i %i %i", (int)newMins.x, (int)newMins.z, (int)newMins.y);
		uint8_t packedRot = 0u;
		RotationMatrixPacked* r = (RotationMatrixPacked*)&packedRot;
		r->nonZeroEntryInSecondRow = 1;
		const core::String& rotationStr = core::string::format("%u", (uint32_t)packedRot);
		wrapBool(saveAttributes({{"_t", translationStr}, {"_r", rotationStr}}, stream))
	} else {
		wrapBool(saveAttributes({}, stream))
	}
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_SIZE(io::FileStream& stream, const voxel::Region& region) {
	ScopedChunkWriter scoped(stream, FourCC('S','I','Z','E'));
	wrapBool(stream.addInt(region.getWidthInVoxels()))
	wrapBool(stream.addInt(region.getDepthInVoxels()))
	wrapBool(stream.addInt(region.getHeightInVoxels()))
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_PACK(io::FileStream& stream, const VoxelVolumes& volumes) {
	int modelCount = 0;
	for (auto& v : volumes) {
		if (skipSaving(v)) {
			continue;
		}
		++modelCount;
	}
	ScopedChunkWriter scoped(stream, FourCC('P','A','C','K'));
	wrapBool(stream.addInt(modelCount))
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_RGBA(io::FileStream& stream) {
	const MaterialColorArray& materialColors = getMaterialColors();
	const int numColors = (int)materialColors.size();
	if (numColors > 256) {
		Log::error("More colors than supported");
		return false;
	}

	ScopedChunkWriter scoped(stream, FourCC('R','G','B','A'));
	for (int i = 0; i < numColors; ++i) {
		const uint32_t rgba = core::Color::getRGBA(materialColors[i]);
		wrapBool(stream.addInt(rgba))
	}
	for (int i = numColors; i < 256; ++i) {
		wrapBool(stream.addInt(0))
	}
	++_chunks;
	return true;
}

bool VoxFormat::saveChunk_XYZI(io::FileStream& stream, const voxel::RawVolume* volume, const voxel::Region& region) {
	ScopedChunkWriter scoped(stream, FourCC('X','Y','Z','I'));

	uint32_t numVoxels = 0;
	const uint64_t numVoxelPos = stream.pos();
	wrapBool(stream.addInt(numVoxels))

	numVoxels = voxelutil::visitVolume(*volume, [&] (int x, int y, int z, const voxel::Voxel& voxel) {
		stream.addByte(region.getWidthInCells() - (x - region.getLowerX()));
		stream.addByte(z - region.getLowerZ());
		stream.addByte(y - region.getLowerY());
		const uint8_t colorIndex = voxel.getColor();
		stream.addByte(colorIndex + 1);
	});
	const uint64_t chunkEndPos = stream.pos();
	wrap(stream.seek(numVoxelPos));
	wrapBool(stream.addInt(numVoxels));
	wrap(stream.seek(chunkEndPos));
	++_chunks;
	return true;
}

bool VoxFormat::saveAttributes(const Attributes& attributes, io::FileStream& stream) const {
	Log::debug("Save %i attributes", (int)attributes.size());
	wrapBool(stream.addInt((uint32_t)attributes.size()))
	for (const auto& e : attributes) {
		const core::String& key = e->first;
		const core::String& value = e->second;
		Log::debug("Save attribute %s: %s", key.c_str(), value.c_str());
		wrapBool(stream.addInt(core::utf8::length(key.c_str())))
		wrapBool(stream.addString(key, false))
		wrapBool(stream.addInt(core::utf8::length(value.c_str())))
		wrapBool(stream.addString(value, false))
	}
	return true;
}

bool VoxFormat::skipSaving(const VoxelVolume& v) const {
	if (v.volume == nullptr) {
		return true;
	}
	const voxel::Region& region = v.volume->region();
	if (region.getDepthInVoxels() > MaxRegionSize || region.getHeightInVoxels() > MaxRegionSize
		|| region.getWidthInVoxels() > MaxRegionSize) {
		Log::warn("a region exceeds the max allowed vox file boundaries: %i:%i:%i",
				region.getWidthInVoxels(), region.getHeightInVoxels(), region.getDepthInVoxels());
		// TODO: cut the big volume into pieces
		return true;
	}
	return false;
}

bool VoxFormat::saveSceneGraph(io::FileStream& stream, const VoxelVolumes& volumes, int modelCount) {
	const NodeId rootNodeId = 0;
	NodeId groupNodeId = rootNodeId + 1;
	wrapBool(saveChunk_nTRN(stream, rootNodeId, groupNodeId, glm::ivec3(0)))

	// this adds a group node with a transform+shape node pair per volume
	wrapBool(saveChunk_nGRP(stream, groupNodeId, modelCount))

	// the first transform node id
	NodeId nodeId = groupNodeId + 1;
	int modelId = 0;
	for (auto& v : volumes) {
		if (skipSaving(v)) {
			continue;
		}

		const voxel::Region& region = v.volume->region();
		// TODO: the translation is broken for saves.
		const glm::ivec3 mins = region.getCenter();
		wrapBool(saveChunk_nTRN(stream, nodeId, nodeId + 1, mins))
		wrapBool(saveChunk_nSHP(stream, nodeId + 1, modelId))

		// transform + shape node per volume
		nodeId += 2;

		++modelId;
	}
	return true;
}

bool VoxFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	reset();

	io::FileStream stream(file.get());

	ScopedHeader scoped(stream, _chunks);
	wrapBool(saveChunk_PACK(stream, volumes))

	int modelId = 0;
	for (auto& v : volumes) {
		if (skipSaving(v)) {
			continue;
		}

		const voxel::Region& region = v.volume->region();
		wrapBool(saveChunk_SIZE(stream, region))
		wrapBool(saveChunk_XYZI(stream, v.volume, region))

		++modelId;
	}

	if (modelId > 0) {
		wrapBool(saveSceneGraph(stream, volumes, modelId))

		for (auto& v : volumes) {
			if (skipSaving(v)) {
				continue;
			}
			wrapBool(saveChunk_LAYR(stream, modelId, v.name, v.visible))
		}
	}

	wrapBool(saveChunk_RGBA(stream))

	// IMAP
	// MATL
	// rOBJ
	// rCAM
	// NOTE
	return true;
}

bool VoxFormat::readAttributes(Attributes& attributes, io::FileStream& stream) const {
	uint32_t cnt;
	wrap(stream.readInt(cnt))
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
		wrap(stream.readInt(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(key)) {
			Log::error("Max string length for key exceeded");
			return false;
		}
		if (!stream.readString(len, key)) {
			Log::error("Failed to read key for dict");
			return false;
		}
		key[len] = '\0';

		// read value
		wrap(stream.readInt(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(value)) {
			Log::error("Max string length for value exceeded");
			return false;
		}
		if (!stream.readString(len, value)) {
			Log::error("Failed to read value for dict");
			return false;
		}
		value[len] = '\0';

		Log::debug("dict entry %i: %s => %s", i, key, value);
		attributes.put(key, value);
	}
	return true;
}

void VoxFormat::initPalette() {
	// 8. Default Palette : if chunk 'RGBA' is absent
	// -------------------------------------------------------------------------------
	static const uint32_t palette[256] = {
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

	_paletteSize = lengthof(palette);
	// convert to our palette
	const MaterialColorArray& materialColors = getMaterialColors();
	for (size_t i = 0u; i < _paletteSize; ++i) {
		const uint32_t p = palette[i];
		const glm::vec4& color = core::Color::fromRGBA(p);
		const int index = core::Color::getClosestMatch(color, materialColors);
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
bool VoxFormat::readChunkHeader(io::FileStream& stream, ChunkHeader& header) const {
	wrap(stream.readInt(header.chunkId))
	wrap(stream.readInt(header.numBytesChunk))
	wrap(stream.readInt(header.numBytesChildrenChunks))
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
bool VoxFormat::loadChunk_RGBA(io::FileStream& stream, const ChunkHeader& header) {
	const MaterialColorArray& materialColors = getMaterialColors();
	for (int i = 0; i <= 254; i++) {
		uint32_t rgba;
		wrap(stream.readInt(rgba))
		const glm::vec4& color = core::Color::fromRGBA(rgba);
		const int index = core::Color::getClosestMatch(color, materialColors);
		Log::trace("rgba %x, r: %f, g: %f, b: %f, a: %f, index: %i, r2: %f, g2: %f, b2: %f, a2: %f",
				rgba, color.r, color.g, color.b, color.a, index, materialColors[index].r, materialColors[index].g, materialColors[index].b, materialColors[index].a);
		_palette[i + 1] = (uint8_t)index;
	}
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
bool VoxFormat::loadChunk_SIZE(io::FileStream& stream, const ChunkHeader& header) {
	// we have to flip the axis here
	uint32_t x, y, z;
	wrap(stream.readInt(x))
	wrap(stream.readInt(z))
	wrap(stream.readInt(y))
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
	_regions.push_back(region);
	if (_regions.size() > 256) {
		Log::error("Found more than 256 layers: %i", (int)_regions.size());
		return false;
	}
	return true;
}

static inline int divFloor(int x, int y) {
	const bool quotientNegative = x < 0;
	return x / y - (x % y != 0 && quotientNegative);
}

glm::ivec3 VoxFormat::calcTransform(const VoxTransform& t, int x, int y, int z, const glm::ivec3& pivot) const {
	const glm::ivec3 c = glm::ivec3(x * 2, y * 2, z * 2) - pivot;
	const glm::ivec3 pos = glm::ivec3(t.rotation * glm::vec3(c.x + 0.5f, c.y + 0.5f, c.z + 0.5f));
	const glm::ivec3 rotated(divFloor(pos.x, 2), divFloor(pos.y, 2), divFloor(pos.z, 2));
	return rotated + t.translation;
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
bool VoxFormat::loadChunk_XYZI(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes) {
	uint32_t numVoxels;
	wrap(stream.readInt(numVoxels))
	Log::debug("Found voxel chunk with %u voxels", numVoxels);
	if (_regions.empty() || _volumeIdx >= (uint32_t)_regions.size()) {
		Log::error("Invalid XYZI chunk without previous SIZE chunk");
		return false;
	}

	const voxel::Region& region = _regions[_volumeIdx];
	const glm::uvec3 size(region.getDimensionsInVoxels());
	const glm::ivec3 pivot = glm::ivec3(size.x - 1, size.z - 1, size.y - 1);
	const glm::ivec3& rmins = region.getLowerCorner();
	const glm::ivec3& rmaxs = region.getUpperCorner();
	const VoxTransform& finalTransform = calculateTransform(_volumeIdx);
	const glm::ivec3& tmins = calcTransform(finalTransform, rmins.x, rmins.z, rmins.y, pivot);
	const glm::ivec3& tmaxs = calcTransform(finalTransform, rmaxs.x, rmaxs.z, rmaxs.y, pivot);
	const glm::ivec3 mins = glm::min(tmins, tmaxs);
	const glm::ivec3 maxs = glm::max(tmins, tmaxs);
	Region translatedRegion{mins.x, mins.z, mins.y, maxs.x, maxs.z, maxs.y};
	const bool applyTransformation = translatedRegion.isValid();
	if (!applyTransformation) {
		translatedRegion = Region(rmins, rmaxs);
		Log::warn("Invalid XYZI chunk region after transform was applied - trying without transformation");
	}
	RawVolume *volume = new RawVolume(translatedRegion);
	int volumeVoxelSet = 0;
	for (uint32_t i = 0; i < numVoxels; ++i) {
		uint8_t x, y, z, colorIndex;
		wrap(stream.readByte(x))
		wrap(stream.readByte(y))
		wrap(stream.readByte(z))
		x = size.x - 1 - x;
		wrap(stream.readByte(colorIndex))
		const uint8_t index = convertPaletteIndex(colorIndex);
		voxel::VoxelType voxelType = voxel::VoxelType::Generic;
		const voxel::Voxel& voxel = voxel::createVoxel(voxelType, index);
		// we have to flip the axis here
		if (applyTransformation) {
			const glm::ivec3 pos = calcTransform(finalTransform, x, y, z, pivot);
			if (volume->setVoxel(pos.x, pos.z, pos.y, voxel)) {
				++volumeVoxelSet;
			}
		} else {
			if (volume->setVoxel(x, z, y, voxel)) {
				++volumeVoxelSet;
			}
		}
	}
	Log::info("Loaded layer %i with %i voxels (%i)", _volumeIdx, numVoxels, volumeVoxelSet);
	if (volumes[_volumeIdx].volume != nullptr) {
		delete volumes[_volumeIdx].volume;
	}
	volumes[_volumeIdx].volume = volume;
	volumes[_volumeIdx].pivot = pivot;
	++_volumeIdx;
	return true;
}

bool VoxFormat::loadChunk_nSHP(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t nodeId;
	wrap(stream.readInt(nodeId))
	Log::debug("shape node: %u", nodeId);
	Attributes nodeAttributes;
	wrapBool(readAttributes(nodeAttributes, stream))
	uint32_t shapeNodeNumModels;
	wrap(stream.readInt(shapeNodeNumModels))
	if (shapeNodeNumModels != 1) {
		Log::error("Shape node chunk contained a numModels value != 1: %i", shapeNodeNumModels);
		return false;
	}
	uint32_t modelId;
	wrap(stream.readInt(modelId));
	if (modelId >= _models.size()) {
		Log::error("ModelId %i exceeds boundaries [%i,%i]", modelId, 0, (int)_models.size());
		return false;
	}
	wrapBool(readAttributes(_models[modelId].attributes, stream))
	_models[modelId].volumeIdx = modelId;
	_models[modelId].nodeId = nodeId;
	_models[modelId].nodeAttributes = nodeAttributes;
	const SceneGraphNode sceneNode{modelId, SceneGraphNodeType::Shape, SceneGraphChildNodes(0)};
	_sceneGraphMap.put(nodeId, sceneNode);
	_leafNodes.push_back(nodeId);
	return true;
}

// 4. Chunk id 'PACK' : if it is absent, only one model in the file
// -------------------------------------------------------------------------------
// # Bytes  | Type       | Value
// -------------------------------------------------------------------------------
// 4        | int        | numModels : num of SIZE and XYZI chunks
// -------------------------------------------------------------------------------
bool VoxFormat::loadChunk_PACK(io::FileStream& stream, const ChunkHeader& header) {
	wrap(stream.readInt(_numModels))
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
bool VoxFormat::loadChunk_MATT(io::FileStream& stream, const ChunkHeader& header) {
	// TODO: this is deprecated - MATL is the v2 version
	uint32_t materialId;
	wrap(stream.readInt(materialId))
	uint32_t materialType;
	wrap(stream.readInt(materialType))
	float materialWeight;
	wrap(stream.readFloat(materialWeight))
	uint32_t materialProperties;
	wrap(stream.readInt(materialProperties))
#if 0
	for (uint32_t i = 0; i < numBytesChunk; ++i) {
		float materialPropertyValue;
		wrap(stream.readFloat(materialPropertyValue))
	}
#endif
	return true;
}

// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
bool VoxFormat::loadChunk_LAYR(io::FileStream& stream, const ChunkHeader& header, VoxelVolumes& volumes) {
	uint32_t layerId;
	wrap(stream.readInt(layerId))
	Attributes attributes;
	// (_name : string)
	// (_hidden : 0/1)
	wrapBool(readAttributes(attributes, stream))
	uint32_t end;
	wrap(stream.readInt(end));
	if ((int)end != -1) {
		Log::error("Unexpected end of LAYR chunk - expected -1, got %i", (int)end);
		return true;
	}
	if (layerId >= (uint32_t)volumes.size()) {
		Log::warn("Invalid layer id found: %i - exceeded limit of %i. Skipping layer",
				(int)layerId, (int)volumes.size());
	} else {
		attributes.get("_name", volumes[layerId].name);
		core::String hidden;
		attributes.get("_hidden", hidden);
		volumes[layerId].visible = hidden.empty() || hidden == "0";
	}
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
bool VoxFormat::parseSceneGraphTranslation(VoxTransform& transform, const Attributes& attributes) const {
	auto trans = attributes.find("_t");
	if (trans == attributes.end()) {
		return true;
	}
	const core::String& translations = trans->second;
	glm::ivec3& v = transform.translation;
	if (SDL_sscanf(translations.c_str(), "%d %d %d", &v.x, &v.y, &v.z) != 3) {
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
bool VoxFormat::parseSceneGraphRotation(VoxTransform &transform, const Attributes &attributes) const {
	auto rot = attributes.find("_r");
	if (rot == attributes.end()) {
		return true;
	}

	const uint8_t packed = core::string::toInt(rot->second);
	const RotationMatrixPacked *packedRot = (const RotationMatrixPacked *)&packed;
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

	glm::mat3x3 rotMat(0.0f);
	// glm is column major - thus we have to flip the col/row indices here
	rotMat[packedRot->nonZeroEntryInFirstRow][0] = packedRot->signInFirstRow ? -1.0f : 1.0f;
	rotMat[packedRot->nonZeroEntryInSecondRow][1] = packedRot->signInSecondRow ? -1.0f : 1.0f;
	rotMat[nonZeroEntryInThirdRow][2] = packedRot->signInThirdRow ? -1.0f : 1.0f;

	for (int i = 0; i < 3; ++i) {
		Log::debug("mat3[%i]: %.2f, %.2f, %.2f", i, rotMat[0][i], rotMat[1][i], rotMat[2][i]);
	}
	transform.rotation = glm::quat_cast(rotMat);

	return true;
}

// (_r : int8) ROTATION in STRING (i.e. "36")
// (_t : int32x3) translation in STRING format separated by spaces (i.e. "-1 10 4"). The anchor for these translations is center of the box.
bool VoxFormat::loadChunk_nTRN(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t nodeId;
	wrap(stream.readInt(nodeId))
	Log::debug("transform node: %u", nodeId);
	Attributes attributes;
	wrapBool(readAttributes(attributes, stream))
	uint32_t childNodeId;
	wrap(stream.readInt(childNodeId))
	uint32_t reserved;
	wrap(stream.readInt(reserved))
	uint32_t layerId;
	wrap(stream.readInt(layerId))
	uint32_t numFrames;
	wrap(stream.readInt(numFrames))
	Log::debug("nTRN chunk: node: %u, childNodeId: %u, layerId: %u, numFrames: %u", nodeId, childNodeId, layerId, numFrames);
	if (numFrames != 1) {
		Log::error("Transform node chunk contained a numFrames value != 1: %i", numFrames);
		return false;
	}
	Attributes transformNodeAttributes;
	wrapBool(readAttributes(transformNodeAttributes, stream))

	VoxTransform transform;
	wrapBool(parseSceneGraphRotation(transform, transformNodeAttributes))
	wrapBool(parseSceneGraphTranslation(transform, transformNodeAttributes))

	SceneGraphChildNodes child(1);
	child[0] = childNodeId;
	const uint32_t arrayIdx = (uint32_t)_transforms.size();
	const SceneGraphNode sceneNode{arrayIdx, SceneGraphNodeType::Transform, child};
	Log::debug("transform child node id: %u, arrayIdx: %u", sceneNode.childNodeIds[0], arrayIdx);
	_sceneGraphMap.put(nodeId, sceneNode);
	_transforms.push_back(transform);
	_parentNodes.put(childNodeId, nodeId);

	return true;
}

bool VoxFormat::loadChunk_nGRP(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t nodeId;
	wrap(stream.readInt(nodeId))
	Log::debug("group node: %u", nodeId);
	Attributes attributes;
	wrapBool(readAttributes(attributes, stream))
	uint32_t numChildren;
	wrap(stream.readInt(numChildren))
	SceneGraphChildNodes children;
	children.reserve(numChildren);
	for (uint32_t i = 0; i < numChildren; ++i) {
		uint32_t child;
		wrap(stream.readInt(child))
		children.push_back((child));
		_parentNodes.put(child, nodeId);
	}
	const SceneGraphNode sceneNode{0, SceneGraphNodeType::Group, children};
	_sceneGraphMap.put(nodeId, sceneNode);
	return true;
}

bool VoxFormat::loadChunk_rCAM(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t cameraId;
	wrap(stream.readInt(cameraId))
	Attributes cameraAttributes;
	// (_mode : string - pers)
	// (_focus : vec(3))
	// (_angle : vec(3))
	// (_radius : int)
	// (_frustum : float)
	// (_fov : int degree)
	wrapBool(readAttributes(cameraAttributes, stream))
	return true;
}

// the rendering setting are not open yet because they are changing frequently.
// But you can still read it since it is just in the DICT format.
bool VoxFormat::loadChunk_rOBJ(io::FileStream& stream, const ChunkHeader& header) {
	Attributes attributes;
	wrapBool(readAttributes(attributes, stream))
	return true;
}

bool VoxFormat::loadChunk_MATL(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t materialId;
	wrap(stream.readInt(materialId))
	Attributes materialAttributes;
	// (_type : str) _diffuse, _metal, _glass, _emit
	// (_weight : float) range 0 ~ 1
	// (_rough : float)
	// (_spec : float)
	// (_ior : float)
	// (_att : float)
	// (_flux : float)
	// (_plastic)
	wrapBool(readAttributes(materialAttributes, stream))
	return true;
}

// Represents the palette "index" map
// TODO: take this into account while mapping the colors - see _palette member
bool VoxFormat::loadChunk_IMAP(io::FileStream& stream, const ChunkHeader& header) {
	for (int i = 0; i < 256; i++) {
		uint8_t paletteIndex;
		wrap(stream.readByte(paletteIndex))
	}
	return true;
}

// Contains all color type names
bool VoxFormat::loadChunk_NOTE(io::FileStream& stream, const ChunkHeader& header) {
	uint32_t numColorNames;
	wrap(stream.readInt(numColorNames))
	for (uint32_t i = 0; i < numColorNames; ++i) {
		uint32_t len;
		char name[1024];
		wrap(stream.readInt(len));
		Log::debug("String of length %i", (int)len);
		if (len >= (uint32_t)sizeof(name)) {
			Log::error("Max string length for color name exceeded");
			return false;
		}
		if (!stream.readString(len, name)) {
			Log::error("Failed to read color name");
			return false;
		}
		name[len] = '\0';
		Log::debug("Found color name %s", name);
	}
	return true;
}

bool VoxFormat::loadFirstChunks(io::FileStream& stream) {
	do {
		ChunkHeader header;
		wrapBool(readChunkHeader(stream, header))
		switch (header.chunkId) {
		case FourCC('M','A','T','L'):
			wrapBool(loadChunk_MATL(stream, header))
			break;
		case FourCC('M','A','T','T'):
			wrapBool(loadChunk_MATT(stream, header))
			break;
		case FourCC('P','A','C','K'):
			wrapBool(loadChunk_PACK(stream, header))
			break;
		case FourCC('R','G','B','A'):
			wrapBool(loadChunk_RGBA(stream, header))
			break;
		case FourCC('I','M','A','P'):
			wrapBool(loadChunk_IMAP(stream, header))
			break;
		case FourCC('N','O','T','E'):
			wrapBool(loadChunk_NOTE(stream, header))
			break;
		case FourCC('r','C','A','M'):
			wrapBool(loadChunk_rCAM(stream, header))
			break;
		case FourCC('r','O','B','J'):
			wrapBool(loadChunk_rOBJ(stream, header))
			break;
		case FourCC('S','I','Z','E'):
			wrapBool(loadChunk_SIZE(stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos))
	} while (stream.remaining() > 0);

	_models.resize(_regions.size());

	return true;
}

bool VoxFormat::loadSecondChunks(io::FileStream& stream, VoxelVolumes& volumes) {
	volumes.resize(_regions.size());
	do {
		ChunkHeader header;
		wrapBool(readChunkHeader(stream, header));
		switch (header.chunkId) {
		case FourCC('L','A','Y','R'):
			wrapBool(loadChunk_LAYR(stream, header, volumes))
			break;
		case FourCC('X','Y','Z','I'):
			wrapBool(loadChunk_XYZI(stream, header, volumes))
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
bool VoxFormat::loadSceneGraph(io::FileStream& stream) {
	do {
		ChunkHeader header;
		wrapBool(readChunkHeader(stream, header))

		switch (header.chunkId) {
		case FourCC('n','G','R','P'):
			wrapBool(loadChunk_nGRP(stream, header))
			break;
		case FourCC('n','T','R','N'):
			wrapBool(loadChunk_nTRN(stream, header))
			break;
		case FourCC('n','S','H','P'):
			wrapBool(loadChunk_nSHP(stream, header))
			break;
		}
		wrap(stream.seek(header.nextChunkPos));
	} while (stream.remaining() > 0);

	return true;
}

VoxFormat::VoxTransform VoxFormat::calculateTransform(uint32_t volumeIdx) const {
	const NodeId nodeId = _models[volumeIdx].nodeId;
	VoxTransform transform;
	applyTransform(transform, nodeId);
	return transform;
}

bool VoxFormat::applyTransform(VoxTransform& transform, NodeId nodeId) const {
	NodeId parent;
	NodeId current = nodeId;
	while (_parentNodes.get(current, parent)) {
		wrapBool(applyTransform(transform, parent))
		current = parent;
	}

	// skip root node
	if (nodeId == 0) {
		return true;
	}

	SceneGraphNode node;
	if (!_sceneGraphMap.get(nodeId, node)) {
		Log::debug("Could not find node %u", nodeId);
		return false;
	}

	if (node.type == SceneGraphNodeType::Transform) {
		if (node.arrayIdx >= _transforms.size()) {
			Log::error("Invalid transform array index found: %u", node.arrayIdx);
			return false;
		}

		const VoxTransform& t = _transforms[node.arrayIdx];
		transform.rotation = glm::normalize(t.rotation * transform.rotation);
		transform.translation += t.rotation * glm::vec3(t.translation);
		Log::debug("Apply translation for node %u (aidx: %u) %i:%i:%i",
				nodeId, node.arrayIdx,
				transform.translation.x, transform.translation.y, transform.translation.z);
	}

	return true;
}

bool VoxFormat::checkVersionAndMagic(io::FileStream& stream) const {
	uint32_t magic;
	wrap(stream.readInt(magic))
	if (magic != FourCC('V','O','X',' ')) {
		Log::error("Could not load vox file: Invalid magic found");
		return false;
	}

	uint32_t version;
	wrap(stream.readInt(version))

	if (version != 150) {
		Log::warn("Vox file loading is only tested for version 150 - but we've found %i", version);
	}
	return true;
}

bool VoxFormat::checkMainChunk(io::FileStream& stream) const {
	ChunkHeader main;
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
bool VoxFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vox file: File doesn't exist");
		return false;
	}

	reset();

	io::FileStream stream(file.get());
	wrapBool(checkVersionAndMagic(stream))
	wrapBool(checkMainChunk(stream))

	const int64_t resetPos = stream.pos();
	wrapBool(loadFirstChunks(stream))
	stream.seek(resetPos);

	wrapBool(loadSceneGraph(stream))
	stream.seek(resetPos);

	wrapBool(loadSecondChunks(stream, volumes))

	return true;
}

void VoxFormat::reset() {
	initPalette();
	_regions.clear();
	_models.clear();
	_sceneGraphMap.clear();
	_transforms.clear();
	_volumeIdx = 0;
	_chunks = 0;
}

#undef wrap
#undef wrapBool

}
