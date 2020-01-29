/**
 * @file
 */

#include "VoxFormat.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/UTF8.h"
#include "voxel/MaterialColor.h"

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load vox file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapAttributes(read) \
	if (read == false) { \
		Log::error("Could not load vox file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapAttributesRead(read) \
	if (read != 0) { \
		Log::error("Could not load vox file attributes: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

struct VoxModel {
	uint32_t nodeId;
	uint32_t modelId;
	std::map<std::string, std::string> attributes;
	std::map<std::string, std::string> nodeAttributes;
};

bool VoxFormat::saveChunk_LAYR(io::FileStream& stream, int layerId, const core::String& name, bool visible) const {
	const core::String attributeName = "_name";
	const core::String attributeVisible = "_visible";
	stream.addInt(FourCC('L','A','Y','R'));
	const int chunkSizeName = sizeof(uint32_t) + attributeName.size();
	const int chunkSizeNameValue = sizeof(uint32_t) + core::utf8::length(name.c_str());
	const int chunkSizeVisible = sizeof(uint32_t) + attributeVisible.size();
	const int chunkSizeVisibleValue = sizeof(uint32_t) + 1;
	const int chunkAttributeSize = sizeof(uint32_t) + chunkSizeName + chunkSizeNameValue
			+ chunkSizeVisible + chunkSizeVisibleValue;
	const uint32_t chunkSize = sizeof(uint32_t) + chunkAttributeSize + sizeof(uint32_t);
	stream.addInt(chunkSize);
	stream.addInt(0);
	const uint64_t posBefore = stream.pos();
	stream.addInt(layerId);
	saveAttributes({{attributeName, name}, {attributeVisible, visible ? "1" : "0"}}, stream);
	stream.addInt((uint32_t)-1); // must always be -1
	const uint64_t posAfter = stream.pos();
	Log::debug("chunk size is: %i", (int)(posAfter - posBefore));
	core_assert_msg(posAfter - posBefore == chunkSize, "posAfter: %i, posBefore: %i, chunkSize: %i",
			(int)posAfter, (int)posBefore, (int)chunkSize);
	return true;
}

bool VoxFormat::saveChunk_nTRN(io::FileStream& stream, int layerId, const voxel::Region& region) const {
	const core::String attributeName = "_t";
	const glm::ivec3& mins = region.getLowerCorner();
	const core::String& translationStr = core::string::format("%i %i %i", mins.x, mins.y, mins.z);
	stream.addInt(FourCC('n','T','R','N'));
	const int chunkFrameTranslationName = sizeof(uint32_t) + attributeName.size();
	const int chunkFrameTranslationValue = sizeof(uint32_t) + translationStr.size();
	const int chunkNodeAttributeSize = sizeof(uint32_t);
	const int chunkFrameAttributeSize = sizeof(uint32_t) + chunkFrameTranslationName + chunkFrameTranslationValue;
	const uint32_t chunkSize = 5 * sizeof(uint32_t) + chunkNodeAttributeSize + chunkFrameAttributeSize;
	stream.addInt(chunkSize);
	stream.addInt(0);
	const uint64_t posBefore = stream.pos();

	stream.addInt(0); // 0 is root?
	saveAttributes({}, stream);
	stream.addInt(0); // child node id
	stream.addInt(-1); // reserved - must be -1
	stream.addInt(layerId);
	stream.addInt(1); // num frames
	saveAttributes({{attributeName, translationStr}}, stream);
	const uint64_t posAfter = stream.pos();
	Log::debug("chunk size is: %i", (int)(posAfter - posBefore));
	core_assert_msg(posAfter - posBefore == chunkSize, "posAfter: %i, posBefore: %i, chunkSize: %i",
			(int)posAfter, (int)posBefore, (int)chunkSize);
	return true;
}

bool VoxFormat::saveChunk_SIZE(io::FileStream& stream, const voxel::Region& region) const {
	// model size
	Log::debug("add SIZE chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('S','I','Z','E'));
	stream.addInt(3 * sizeof(uint32_t));
	stream.addInt(0);
	stream.addInt(region.getWidthInVoxels());
	stream.addInt(region.getDepthInVoxels());
	stream.addInt(region.getHeightInVoxels());
	return true;
}

bool VoxFormat::saveChunk_RGBA(io::FileStream& stream) const {
	Log::debug("add RGBA chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('R','G','B','A'));
	const MaterialColorArray& materialColors = getMaterialColors();
	const int numColors = materialColors.size();
	if (numColors > 256) {
		Log::error("More colors than supported");
		return false;
	}
	stream.addInt(256 * sizeof(uint32_t));
	stream.addInt(0);

	for (int i = 0; i < numColors; ++i) {
		const uint32_t rgba = core::Color::getRGBA(materialColors[i]);
		stream.addInt(rgba);
	}
	for (int i = numColors; i < 256; ++i) {
		stream.addInt(0);
	}
	return true;
}

bool VoxFormat::saveChunk_XYZI(io::FileStream& stream, const voxel::RawVolume* volume, const voxel::Region& region) const {
	// voxel data
	Log::debug("add XYZI chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('X','Y','Z','I'));
	uint32_t numVoxels = 0;
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				const voxel::Voxel& voxel = volume->voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				++numVoxels;
			}
		}
	}

	stream.addInt(numVoxels * 4 + sizeof(uint32_t));
	stream.addInt(0);

	stream.addInt(numVoxels);
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				const voxel::Voxel& voxel = volume->voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				stream.addByte(x - region.getLowerX());
				stream.addByte(z - region.getLowerZ());
				stream.addByte(y - region.getLowerY());
				const uint8_t colorIndex = voxel.getColor();
				stream.addByte(colorIndex + 1);
			}
		}
	}
	return true;
}

bool VoxFormat::saveAttributes(const std::map<std::string, std::string>& attributes, io::FileStream& stream) const {
	Log::debug("Save %i attributes", (int)attributes.size());
	stream.addInt((uint32_t)attributes.size());
	for (const auto& e : attributes) {
		const core::String& key = e.first;
		const core::String& value = e.second;
		Log::debug("Save attribute %s: %s", key.c_str(), value.c_str());
		stream.addInt(core::utf8::length(key.c_str()));
		stream.addString(key, false);
		stream.addInt(core::utf8::length(value.c_str()));
		stream.addString(value, false);
	}
	return true;
}

bool VoxFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	stream.addInt(FourCC('V','O','X',' '));
	stream.addInt(150);
	stream.addInt(FourCC('M','A','I','N'));
	stream.addInt(0);
	// this is filled at the end - once we know the final size of the main chunk children
	int64_t numBytesMainChunkPos = stream.pos();
	stream.addInt(0);

	int64_t headerSize = stream.pos();
	Log::debug("headersize is: %i", (int)headerSize);

	int layerId = 0;
	for (auto& v : volumes) {
		const voxel::Region& region = v.volume->region();
		if (region.getDepthInVoxels() >= 256 || region.getHeightInVoxels() >= 256
			|| region.getWidthInVoxels() >= 256) {
			Log::warn("a region exceeds the max allowed vox file boundaries - layer %i is not saved", layerId);
			continue;
		}
		if (!saveChunk_SIZE(stream, region)) {
			return false;
		}
		if (!saveChunk_LAYR(stream, layerId, v.name, v.visible)) {
			return false;
		}
		if (!saveChunk_nTRN(stream, layerId, region)) {
			return false;
		}
		if (!saveChunk_XYZI(stream, v.volume, region)) {
			return false;
		}

		++layerId;
	}

	if (layerId == 0) {
		return false;
	}

	if (!saveChunk_RGBA(stream)) {
		return false;
	}

	// magic, version, main chunk, main chunk size, main chunk child size
	const int64_t mainChildChunkSize = stream.pos() - headerSize;
	if (stream.seek(numBytesMainChunkPos) == -1) {
		Log::error("Failed to seek in the stream to pos %i", (int)numBytesMainChunkPos);
		return false;
	}
	stream.addInt(mainChildChunkSize);

	return true;
}

bool VoxFormat::readAttributes(std::map<std::string, std::string>& attributes, io::FileStream& stream) const {
	uint32_t cnt;
	wrapAttributesRead(stream.readInt(cnt))
	Log::debug("Reading %i keys in the dict", cnt);
	for (uint32_t i = 0; i < cnt; ++i) {
		char key[1024];
		char value[1024];
		uint32_t len;

		// read key
		wrapAttributesRead(stream.readInt(len));
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
		wrapAttributesRead(stream.readInt(len));
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
		attributes.insert(std::make_pair(key, value));
	}
	return true;
}

bool VoxFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vox file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());

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

	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic = FourCC('V','O','X',' ');
	if (header != headerMagic) {
		Log::error("Could not load vox file: Invalid magic found (%u vs %u)", header, headerMagic);
		return false;
	}

	uint32_t version;
	wrap(stream.readInt(version))

	if (version != 150) {
		Log::warn("Vox file loading is only tested for version 150 - but we've found %i", version);
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

	uint32_t mainChunk;
	wrap(stream.readInt(mainChunk))
	// 3. Chunk id 'MAIN' : the root chunk and parent chunk of all the other chunks
	if (mainChunk != FourCC('M','A','I','N')) {
		Log::error("Could not load vox file: Invalid magic for main chunk found");
		return false;
	}

	uint32_t numBytesMainChunk;
	wrap(stream.readInt(numBytesMainChunk))
	uint32_t numBytesMainChildrenChunks;
	wrap(stream.readInt(numBytesMainChildrenChunks))

	if (stream.remaining() < numBytesMainChildrenChunks) {
		Log::error("Could not load vox file: Incomplete file");
		return false;
	}

	const int64_t resetPos = stream.pos();

	uint32_t numModels = 1;

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

	const int paletteSize = lengthof(palette);
	_palette.reserve(paletteSize);
	_paletteSize = paletteSize;
	// convert to our palette
	const MaterialColorArray& materialColors = getMaterialColors();
	for (int i = 0; i < paletteSize; ++i) {
		const uint32_t p = palette[i];
		const glm::vec4& color = core::Color::fromRGBA(p);
		const int index = core::Color::getClosestMatch(color, materialColors);
		_palette[i] = index;
	}

	std::vector<Region> regions;

	do {
		uint32_t chunkId;
		wrap(stream.readInt(chunkId))
		uint32_t numBytesChunk;
		wrap(stream.readInt(numBytesChunk))
		uint32_t numBytesChildrenChunks;
		wrap(stream.readInt(numBytesChildrenChunks))
		const int64_t currentChunkPos = stream.pos();
		const int64_t nextChunkPos = currentChunkPos + numBytesChunk + numBytesChildrenChunks;
		if (chunkId == FourCC('R','G','B','A')) {
			Log::debug("Found palette chunk with %u bytes (currentPos: %i, nextPos: %i)", numBytesChunk, (int)currentChunkPos, (int)nextChunkPos);
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
			for (int i = 0; i <= 254; i++) {
				uint32_t rgba;
				wrap(stream.readInt(rgba))
				const glm::vec4& color = core::Color::fromRGBA(rgba);
				const int index = core::Color::getClosestMatch(color, materialColors);
				Log::trace("rgba %x, r: %f, g: %f, b: %f, a: %f, index: %i, r2: %f, g2: %f, b2: %f, a2: %f",
						rgba, color.r, color.g, color.b, color.a, index, materialColors[index].r, materialColors[index].g, materialColors[index].b, materialColors[index].a);
				_palette[i + 1] = (uint8_t)index;
			}
			break;
		} else if (chunkId == FourCC('S','I','Z','E')) {
			Log::debug("Found size chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 5. Chunk id 'SIZE' : model size
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | size x
			// 4        | int        | size y
			// 4        | int        | size z : gravity direction
			// -------------------------------------------------------------------------------
			// we have to flip the axis here
			uint32_t x, y, z;
			wrap(stream.readInt(x))
			wrap(stream.readInt(z))
			wrap(stream.readInt(y))
			glm::ivec3 maxsregion((int32_t)(x) - 1, (int32_t)(y) - 1, (int32_t)(z) - 1);
			Log::debug("Found size chunk: (%u:%u:%u)", x, y, z);
			Region region(glm::ivec3(0), maxsregion);
			regions.push_back(region);
		}
		Log::debug("Set next chunk pos to %i of %i", (int)nextChunkPos, (int)stream.size());
		wrap(stream.seek(nextChunkPos));
	} while (stream.remaining() > 0);

	stream.seek(resetPos);

	std::vector<VoxModel> models;
	models.resize(regions.size());
	volumes.resize(regions.size());
	int volumeIdx = 0;
	do {
		uint32_t chunkId;
		wrap(stream.readInt(chunkId))
		uint32_t numBytesChunk;
		wrap(stream.readInt(numBytesChunk))
		uint32_t numBytesChildrenChunks;
		wrap(stream.readInt(numBytesChildrenChunks))
		const int64_t currentChunkPos = stream.pos();
		const int64_t nextChunkPos = currentChunkPos + numBytesChunk + numBytesChildrenChunks;

		if (chunkId == FourCC('X','Y','Z','I')) {
			Log::debug("Found voxel chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 6. Chunk id 'XYZI' : model voxels
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | numVoxels (N)
			// 4 x N    | int        | (x, y, z, colorIndex) : 1 byte for each component
			// -------------------------------------------------------------------------------
			uint32_t numVoxels;
			wrap(stream.readInt(numVoxels))
			Log::debug("Found voxel chunk with %u voxels", numVoxels);
			if (regions.empty() || volumeIdx >= (int)regions.size()) {
				Log::error("Invalid XYZI chunk without previous SIZE chunk");
				return false;
			}
			RawVolume *volume = new RawVolume(regions[volumeIdx]);
			int volumeVoxelSet = 0;
			for (uint32_t i = 0; i < numVoxels; ++i) {
				// we have to flip the axis here
				uint8_t x, y, z, colorIndex;
				wrap(stream.readByte(x))
				wrap(stream.readByte(z))
				wrap(stream.readByte(y))
				wrap(stream.readByte(colorIndex))
				const uint8_t index = convertPaletteIndex(colorIndex);
				voxel::VoxelType voxelType = voxel::VoxelType::Generic;
				const voxel::Voxel& voxel = voxel::createVoxel(voxelType, index);
				if (volume->setVoxel(x, y, z, voxel)) {
					++volumeVoxelSet;
				}
			}
			Log::info("Loaded layer %i with %i voxels (%i)", volumeIdx, numVoxels, volumeVoxelSet);
			if (volumes[volumeIdx].volume != nullptr) {
				delete volumes[volumeIdx].volume;
			}
			volumes[volumeIdx].volume = volume;
			volumes[volumeIdx].pivot = volume->region().getCentre();
			++volumeIdx;
		} else if (chunkId == FourCC('n','S','H','P')) {
			// Shape Node Chunk
			uint32_t nodeId;
			wrap(stream.readInt(nodeId)) // 0 is root?
			std::map<std::string, std::string> nodeAttributes;
			wrapAttributes(readAttributes(nodeAttributes, stream))
			uint32_t shapeNodeNumModels;
			wrap(stream.readInt(shapeNodeNumModels)) // must be 1
			if (shapeNodeNumModels != 1) {
				Log::error("Shape node chunk contained a numModels value != 1: %i", shapeNodeNumModels);
				return false;
			}
			// there can be multiple SIZE and XYZI chunks for multiple models; model id is their index in the stored order
			uint32_t modelId;
			wrap(stream.readInt(modelId));
			if (modelId >= models.size()) {
				Log::error("ModelId %i exceeds boundaries [%i,%i]", modelId, 0, (int)models.size());
				return false;
			}
			wrapAttributes(readAttributes(models[modelId].attributes, stream))
			models[modelId].modelId = modelId;
			models[modelId].nodeId = nodeId;
			models[modelId].nodeAttributes = std::move(nodeAttributes);
		}
		Log::debug("Set next chunk pos to %i of %i", (int)nextChunkPos, (int)stream.size());
		wrap(stream.seek(nextChunkPos));
	} while (stream.remaining() > 0);

	stream.seek(resetPos);

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

	do {
		uint32_t chunkId;
		wrap(stream.readInt(chunkId))
		uint32_t numBytesChunk;
		wrap(stream.readInt(numBytesChunk))
		uint32_t numBytesChildrenChunks;
		wrap(stream.readInt(numBytesChildrenChunks))
		const int64_t currentChunkPos = stream.pos();
		const int64_t nextChunkPos = currentChunkPos + numBytesChunk + numBytesChildrenChunks;

		if (chunkId == FourCC('P','A','C','K')) {
			Log::debug("Found pack chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 4. Chunk id 'PACK' : if it is absent, only one model in the file
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | numModels : num of SIZE and XYZI chunks
			// -------------------------------------------------------------------------------
			wrap(stream.readInt(numModels))
		} else if (chunkId == FourCC('M','A','T','T')) {
			Log::debug("Found material chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
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
			// TODO:
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
		} else if (chunkId == FourCC('R','G','B','A') || chunkId == FourCC('S','I','Z','E') || chunkId == FourCC('X','Y','Z','I') || chunkId == FourCC('n','S','H','P')) {
			// already loaded
		} else if (chunkId == FourCC('L','A','Y','R')) {
			// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
			uint32_t layerId;
			wrap(stream.readInt(layerId))
			std::map<std::string, std::string> attributes;
			wrapAttributes(readAttributes(attributes, stream))
			uint32_t end;
			wrap(stream.readInt(end));
			if ((int)end != -1) {
				Log::error("Unexpected end of LAYR chunk - expected -1, got %i", (int)end);
				return true;
			}
			if (layerId >= (uint32_t)volumes.size()) {
				Log::warn("Invalid layer id found: %i - exceeded limit of %i. Skipping layer with name '%s'",
						(int)layerId, (int)volumes.size(), attributes["_name"].c_str());
			} else {
				volumes[layerId].name = attributes["_name"];
				const core::String& hidden = attributes["_hidden"];
				volumes[layerId].visible = hidden.empty() || hidden == "0";
			}
		} else if (chunkId == FourCC('n','T','R','N')) {
			Log::debug("Found nTRN chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)",
					numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			uint32_t nodeId;
			wrap(stream.readInt(nodeId)) // 0 is root?
			std::map<std::string, std::string> attributes;
			// (_name : string)
			// (_hidden : 0/1)
			wrapAttributes(readAttributes(attributes, stream))
			uint32_t childNodeId;
			wrap(stream.readInt(childNodeId))
			uint32_t reserved;
			wrap(stream.readInt(reserved))
			uint32_t layerId;
			wrap(stream.readInt(layerId))
			uint32_t numFrames;
			wrap(stream.readInt(numFrames))
			Log::debug("nTRN chunk: childNodeId: %u, layerId: %u, numFrames: %u", childNodeId, layerId, numFrames);
			for (uint32_t i = 0; i < numFrames; ++i) {
				std::map<std::string, std::string> transformNodeAttributes;
				// (_r : int8) ROTATION
				// (_t : int32x3) translation
				//
				// ROTATION type
				//
				// store a row-major rotation in the bits of a byte
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
				//
				wrapAttributes(readAttributes(transformNodeAttributes, stream))
				auto rot = transformNodeAttributes.find("_r");
				if (rot != transformNodeAttributes.end()) {
					Log::warn("nTRN chunk not yet completely supported: _r not yet parsed");
				}
				auto trans = transformNodeAttributes.find("_t");
				if (trans != transformNodeAttributes.end()) {
					const core::String& translations = trans->second;
					int x, y, z;
					if (sscanf(translations.c_str(), "%d %d %d", &x, &y, &z) == 3) {
						for (auto& m : models) {
							if (m.nodeId == nodeId) {
								volumes[m.modelId].volume->translate(glm::ivec3(x, y, z));
								break;
							}
						}
						Log::debug("nTRN chunk not yet completely supported: translation %i:%i:%i", x, y, z);
					} else {
						Log::error("Failed to parse translation %s", translations.c_str());
					}
				}
			}
			// TODO:
		} else if (chunkId == FourCC('n','G','R','P')) {
			Log::warn("nGRP chunk not yet supported");
			uint32_t nodeId;
			wrap(stream.readInt(nodeId)) // 0 is root?
			std::map<std::string, std::string> attributes;
			wrapAttributes(readAttributes(attributes, stream))
			uint32_t numChildren;
			wrap(stream.readInt(numChildren))
			std::vector<uint32_t> children;
			children.reserve(numChildren);
			for (uint32_t i = 0; i < numChildren; ++i) {
				uint32_t child;
				wrap(stream.readInt(child))
				children.push_back((child));
			}
			// TODO
		} else if (chunkId == FourCC('M','A','T','L')) {
			uint32_t materialId;
			wrap(stream.readInt(materialId))
			std::map<std::string, std::string> materialAttributes;
			// (_type : str) _diffuse, _metal, _glass, _emit
			// (_weight : float) range 0 ~ 1
			// (_rough : float)
			// (_spec : float)
			// (_ior : float)
			// (_att : float)
			// (_flux : float)
			// (_plastic)
			wrapAttributes(readAttributes(materialAttributes, stream))
			// TODO
		} else if (chunkId == FourCC('r','O','B','J')) {
			Log::warn("rOBJ chunk not yet supported");
			std::map<std::string, std::string> attributes;
			wrapAttributes(readAttributes(attributes, stream))
		} else {
			uint8_t out[4];
			FourCCRev(out, chunkId);
			Log::warn("Unknown chunk in vox (%c %c %c %c) file: %u with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)",
					out[0], out[1], out[2], out[3], chunkId, numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
		}
		Log::debug("Set next chunk pos to %i of %i", (int)nextChunkPos, (int)stream.size());
		wrap(stream.seek(nextChunkPos));
	} while (stream.remaining() > 0);

	return true;
}

#undef wrap
#undef wrapAttributes
#undef wrapAttributesRead

}
