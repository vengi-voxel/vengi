/**
 * @file
 */

#include "QBTFormat.h"
#include "core/Common.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/GLM.h"

namespace voxel {

static const bool MergeCompounds = true;

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapBool(read) \
	if (read == false) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

bool QBTFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	return false;
}

bool QBTFormat::skipNode(io::FileStream& stream) {
	// node type, can be ignored
	uint32_t nodeTypeId;
	wrap(stream.readInt(nodeTypeId));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));
	stream.skip(dataSize);
	return true;
}

/**
 * Compound Node
 * TypeID 4 bytes, uint = 2
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this node)
 * NameLength  4 bytes
 * Name NameLength bytes, char
 * Position X, Y, Z 3 * 4 bytes, int, position relative to parent node
 * LocalScale X, Y, Z 3 * 4 bytes, uint
 * Pivot X, Y, Z 3 * 4 bytes, float
 * Size X, Y, Z 3 * 4 bytes, uint
 * CompoundVoxelDataSize 4 bytes, uint
 * CompoundVoxelData VoxelDataSize bytes, zlib compressed voxel data
 * ChildCount 4 bytes, uint, number of child nodes
 * Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadCompound(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	if (!loadMatrix(stream, volumes)) {
		return false;
	}
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	for (uint32_t i = 0; i < childCount; ++i) {
		if (MergeCompounds) {
			// if you don't need the datatree you can skip child nodes
			if (!skipNode(stream)) {
				return false;
			}
		} else {
			if (!loadNode(stream, volumes)) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Matrix Node
 * TypeID 4 bytes, uint = 0
 * DataSize 4 bytes, uint, number of bytes used for this node (excluding TypeID and DataSize)
 * NameLength  4 bytes
 * Name NameLength bytes, char
 * Position X, Y, Z 3 * 4 bytes, int, position relative to parent node
 * LocalScale X, Y, Z 3 * 4 bytes, uint
 * Pivot X, Y, Z 3 * 4 bytes, float
 * Size X, Y, Z 3 * 4 bytes, uint
 * VoxelDataSize 4 bytes, uint
 * VoxelData VoxelDataSize bytes, zlib compressed voxel data
 *
 * Voxel Data
 * Voxel data is stored in a 3D grid. The data is compressed using zlib and stored in X, Y, Z with Y running fastest and X running slowest. Each voxel uses 4 bytes:
 * RGBM. RGB stores true color information and M the visibility Mask.
 *
 * If a color map is included then the R byte references to a color of the color map. In this case the G and B bytes may contain additional secondary data references.
 *
 * The M byte is used to store visibility of the 6 faces of a voxel and whether as voxel is solid or air. If M is bigger than 0 then the voxel is solid. Even when a voxel
 * is solid is may not be needed to be rendered because it is a core voxel that is surrounded by 6 other voxels and thus invisible. If M = 1 then the voxel is a core voxel.
 */
bool QBTFormat::loadMatrix(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	char buf[1024];
	uint32_t nameLength;
	wrap(stream.readInt(nameLength));
	if ((size_t)nameLength >= sizeof(buf)) {
		return false;
	}
	wrapBool(stream.readString(nameLength, buf));
	buf[nameLength] = '\0';
	Log::debug("Matrix name: %s", buf);
	glm::ivec3 position;
	glm::uvec3 localScale;
	glm::vec3 pivot;
	glm::uvec3 size;
	wrap(stream.readInt((uint32_t&)position.x));
	wrap(stream.readInt((uint32_t&)position.y));
	wrap(stream.readInt((uint32_t&)position.z));
	wrap(stream.readInt(localScale.x));
	wrap(stream.readInt(localScale.y));
	wrap(stream.readInt(localScale.z));
	wrap(stream.readFloat(pivot.x));
	wrap(stream.readFloat(pivot.y));
	wrap(stream.readFloat(pivot.z));
	wrap(stream.readInt(size.x));
	wrap(stream.readInt(size.y));
	wrap(stream.readInt(size.z));

	uint32_t voxelDataSize;
	wrap(stream.readInt(voxelDataSize));
	Log::info("Matrix size: %u:%u:%u with %u bytes", size.x, size.y, size.z, voxelDataSize);
	if (voxelDataSize > 0xFFFFFF) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::greaterThan(size, glm::uvec3(2048)))) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	uint8_t* voxelData = new uint8_t[voxelDataSize];
	wrap(stream.readBuf(voxelData, voxelDataSize));

	const uint32_t voxelDataSizeDecompressed = size.x * size.y * size.z * 4;
	uint8_t* voxelDataDecompressed = new uint8_t[voxelDataSizeDecompressed];

	if (!core::zip::uncompress(voxelData, voxelDataSize, voxelDataDecompressed, voxelDataSizeDecompressed)) {
		Log::error("Could not load qbt file: Failed to extract zip data");
		if (voxelDataSize >= 4) {
			Log::debug("First 4 bytes: 0x%x 0x%x 0x%x 0x%x", voxelData[0], voxelData[1], voxelData[2], voxelData[3]);
		}
		delete [] voxelData;
		delete [] voxelDataDecompressed;
		return false;
	}
	const voxel::Region region(0, 0, 0, size.x, size.y, size.z);
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	uint32_t byteCounter = 0u;
	for (uint32_t z = 0; z < size.z; z++) {
		for (uint32_t y = 0; y < size.y; y++) {
			for (uint32_t x = 0; x < size.x; x++) {
				const uint32_t red   = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 0;
				const uint32_t green = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 8;
				const uint32_t blue  = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 16;
				const uint32_t alpha = ((uint32_t)255) << 24;
#if 0
				const uint32_t mask  = ((uint32_t)voxelDataDecompressed[byteCounter++]);
#else
				++byteCounter;
#endif

				const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
				const uint8_t index = findClosestIndex(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				volume->setVoxel(x, y, z, voxel);
			}
		}
	}
	delete [] voxelData;
	delete [] voxelDataDecompressed;
	volumes.push_back(volume);
	return true;
}

/**
 * Model Node
 * TypeID 4 bytes, uint = 1
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this node)
 * ChildCount 4 bytes, uint, number of child nodes
 * Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadModel(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	Log::info("Found %u children", childCount);
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, volumes)) {
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadNode(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t nodeTypeID;
	wrap(stream.readInt(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));

	switch (nodeTypeID) {
	case 0:
		Log::info("Found matrix");
		if (!loadMatrix(stream, volumes)) {
			return false;
		}
		Log::info("Matrix of size %u loaded", dataSize);
		break;
	case 1:
		Log::info("Found model");
		if (!loadModel(stream, volumes)) {
			return false;
		}
		Log::info("Model of size %u loaded", dataSize);
		break;
	case 2:
		Log::info("Found compound");
		if (!loadCompound(stream, volumes)) {
			return false;
		}
		Log::info("Compound of size %u loaded", dataSize);
		break;
	default:
		Log::debug("Skip unknown node type %u of size %u", nodeTypeID, dataSize);
		// skip node if unknown
		stream.skip(dataSize);
		break;
	}
	return true;
}

bool QBTFormat::loadColorMap(io::FileStream& stream) {
	uint32_t colorCount;
	wrap(stream.readInt(colorCount));
	Log::debug("Load color map with %u colors", colorCount);
	if (colorCount > 0xFFFF) {
		Log::error("Sanity check for max colors failed");
		return false;
	}
	_paletteSize = 0;
	_palette.reserve(colorCount);
	for (uint32_t i = 0; i < colorCount; ++i) {
		uint8_t colorByteR;
		uint8_t colorByteG;
		uint8_t colorByteB;
		uint8_t colorByteVisMask;
		wrap(stream.readByte(colorByteR));
		wrap(stream.readByte(colorByteG));
		wrap(stream.readByte(colorByteB));
		wrap(stream.readByte(colorByteVisMask));

		const uint32_t red   = ((uint32_t)colorByteR) << 24;
		const uint32_t green = ((uint32_t)colorByteG) << 16;
		const uint32_t blue  = ((uint32_t)colorByteB) << 8;
		const uint32_t alpha = ((uint32_t)255) << 0;

		const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
		const uint8_t index = findClosestIndex(color);
		_palette[i] = index;
	}
	_paletteSize = colorCount;
	return true;
}

bool QBTFormat::loadFromStream(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic = FourCC('Q','B',' ','2');
	if (header != headerMagic) {
		Log::error("Could not load qbt file: Invalid magic found (%u vs %u)", header, headerMagic);
		return false;
	}

	/**
	 * Header
	 * Magic  4 bytes must be 0x32204251 = "QB 2"
	 * VersionMajor 1 byte, currently = 1
	 * VersionMinor 1 byte, currently = 0
	 * GlobalScale X, Y, Z 3 * 4 bytes, float, normally 1, 1, 1, can be used in case voxels are not cubes (e.g. Lego Bricks)
	 */
	uint8_t versionMajor;
	wrap(stream.readByte(versionMajor))

	uint8_t versionMinor;
	wrap(stream.readByte(versionMinor))

	Log::info("QBT with version %i:%i", (int)versionMajor, (int)versionMinor);

	glm::vec3 globalScale;
	wrap(stream.readFloat(globalScale.x));
	wrap(stream.readFloat(globalScale.y));
	wrap(stream.readFloat(globalScale.z));

	for (int i = 0; i < 2; ++i) {
		char buf[8];
		wrapBool(stream.readString(sizeof(buf), buf));
		if (0 == memcmp(buf, "COLORMAP", 7)) {
			/**
			 * Color Map
			 * SectionCaption 8 bytes = "COLORMAP"
			 * ColorCount 4 bytes, uint, if this value is 0 then no color map is used
			 * Colors ColorCount * 4 bytes, rgba
			 */
			if (!loadColorMap(stream)) {
				return false;
			}
			Log::info("Color map loaded");
		} else if (0 == memcmp(buf, "DATATREE", 8)) {
			/**
			 * Data Tree
			 * SectionCaption 8 bytes = "DATATREE"
			 * RootNode, can currently either be Model, Compound or Matrix
			 */
			if (!loadNode(stream, volumes)) {
				return false;
			}
		} else {
			Log::info("Unknown section found: %c%c%c%c%c%c%c%c",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
			return false;
		}
	}
	return true;
}

std::vector<RawVolume*> QBTFormat::loadGroups(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qbt file: File doesn't exist");
		return std::vector<RawVolume*>();
	}
	io::FileStream stream(file.get());
	std::vector<RawVolume*> volumes;
	if (!loadFromStream(stream, volumes)) {
		return std::vector<RawVolume*>();
	}
	return volumes;
}


#undef wrap
#undef wrapBool

}
