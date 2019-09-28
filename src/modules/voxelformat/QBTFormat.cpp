/**
 * @file
 */

#include "QBTFormat.h"
#include "core/Common.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "voxel/MaterialColor.h"

namespace voxel {

static const bool MergeCompounds = true;

#define wrapSave(write) \
	if (write == false) { \
		Log::error("Could not save qbt file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveFree(write) \
	if (write == false) { \
		Log::error("Could not save qbt file: " CORE_STRINGIFY(write) " failed"); \
		delete[] compressedBuf; \
		delete[] zlibBuffer; \
		return false; \
	}

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if (read == false) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left (line %i)", (int)stream.remaining(), (int)__LINE__); \
		return false; \
	}

bool QBTFormat::saveMatrix(io::FileStream& stream, const VoxelVolume& volume, bool colorMap) const {
	const voxel::Region& region = volume.volume->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	const int zlibBufSize = size.x * size.y * size.z * sizeof(uint32_t);
	core_assert(zlibBufSize > 0);
	uint8_t * const zlibBuffer = new uint8_t[zlibBufSize];
	const uint32_t compressedBufSize = core::zip::compressBound(zlibBufSize);
	uint8_t *compressedBuf = new uint8_t[compressedBufSize];

	uint8_t* zlibBuf = zlibBuffer;
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			for (int y = mins.y; y <= maxs.y; ++y) {
				const Voxel& voxel = volume.volume->voxel(x, y, z);
				if (isAir(voxel.getMaterial())) {
					*zlibBuf++ = (uint8_t)0;
					*zlibBuf++ = (uint8_t)0;
					*zlibBuf++ = (uint8_t)0;
					*zlibBuf++ = (uint8_t)0;
					continue;
				}
				if (colorMap) {
					*zlibBuf++ = voxel.getColor();
					*zlibBuf++ = 0;
					*zlibBuf++ = 0;
				} else {
					const glm::vec4& voxelColor = getColor(voxel);
					const uint8_t red = voxelColor.r * 255.0f;
					const uint8_t green = voxelColor.g * 255.0f;
					const uint8_t blue = voxelColor.b * 255.0f;
					//const uint8_t alpha = voxelColor.a * 255.0f;
					*zlibBuf++ = red;
					*zlibBuf++ = green;
					*zlibBuf++ = blue;
				}
				*zlibBuf++ = 0xff;
			}
		}
	}

	size_t realBufSize = 0;
	if (!core::zip::compress(zlibBuffer, zlibBufSize, compressedBuf, compressedBufSize, &realBufSize)) {
		delete[] compressedBuf;
		delete[] zlibBuffer;
		return false;
	}

	wrapSaveFree(stream.addString("DATATREE", false));
	wrapSaveFree(stream.addInt(0)); // node type matrix
	const int datasize = 14 * sizeof(uint32_t) + realBufSize;
	wrapSaveFree(stream.addInt(datasize));
	const int nameLength = volume.name.size();
	wrapSaveFree(stream.addInt(nameLength));
	wrapSaveFree(stream.addString(volume.name, false));

	wrapSaveFree(stream.addInt(mins.x));
	wrapSaveFree(stream.addInt(mins.y));
	wrapSaveFree(stream.addInt(mins.z));

	glm::uvec3 localScale { 1 };
	wrapSaveFree(stream.addInt(localScale.x));
	wrapSaveFree(stream.addInt(localScale.y));
	wrapSaveFree(stream.addInt(localScale.z));

	glm::vec3 pivot { 0 };
	wrapSaveFree(stream.addFloat(pivot.x));
	wrapSaveFree(stream.addFloat(pivot.y));
	wrapSaveFree(stream.addFloat(pivot.z));

	wrapSaveFree(stream.addInt(size.x));
	wrapSaveFree(stream.addInt(size.y));
	wrapSaveFree(stream.addInt(size.z));

	Log::debug("save %i compressed bytes", (int)realBufSize);
	wrapSaveFree(stream.addInt(realBufSize));
	wrapSaveFree(stream.append(compressedBuf, realBufSize));

	delete[] compressedBuf;
	delete[] zlibBuffer;

	return true;
}

bool QBTFormat::saveColorMap(io::FileStream& stream) const {
	wrapSave(stream.addString("COLORMAP", false));
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	wrapSave(stream.addInt((uint32_t)materialColors.size()));
	for (const glm::vec4& c : materialColors) {
		const uint32_t rgba = core::Color::getRGBA(c);
		wrapSave(stream.addInt(rgba));
	}
	return true;
}

bool QBTFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	wrapSave(stream.addInt(FourCC('Q','B',' ','2')))
	wrapSave(stream.addByte(1));
	wrapSave(stream.addByte(0));
	wrapSave(stream.addFloat(1.0f));
	wrapSave(stream.addFloat(1.0f));
	wrapSave(stream.addFloat(1.0f));
	bool colorMap = false;
	if (colorMap) {
		if (!saveColorMap(stream)) {
			return false;
		}
	}
	for (auto& v : volumes) {
		if (!saveMatrix(stream, v, colorMap)) {
			return false;
		}
	}
	return true;
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
bool QBTFormat::loadCompound(io::FileStream& stream, VoxelVolumes& volumes) {
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
bool QBTFormat::loadMatrix(io::FileStream& stream, VoxelVolumes& volumes) {
	char name[1024];
	uint32_t nameLength;
	wrap(stream.readInt(nameLength));
	if ((size_t)nameLength >= sizeof(name)) {
		return false;
	}
	wrapBool(stream.readString(nameLength, name));
	name[nameLength] = '\0';
	Log::debug("Matrix name: %s", name);
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
	Log::debug("Matrix size: %u:%u:%u with %u bytes", size.x, size.y, size.z, voxelDataSize);
	if (voxelDataSize == 0) {
		Log::warn("Empty voxel chunk found");
		return false;
	}
	if (voxelDataSize > 0xFFFFFF) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::greaterThan(size, glm::uvec3(2048)))) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(size, glm::uvec3(1)))) {
		Log::warn("Size of matrix results in empty space");
		return false;
	}
	uint8_t* voxelData = new uint8_t[voxelDataSize];
	wrap(stream.readBuf(voxelData, voxelDataSize));

	const uint32_t voxelDataSizeDecompressed = size.x * size.y * size.z * sizeof(uint32_t);
	core_assert(voxelDataSizeDecompressed > 0);
	uint8_t* voxelDataDecompressed = new uint8_t[voxelDataSizeDecompressed * 2];

	if (!core::zip::uncompress(voxelData, voxelDataSize, voxelDataDecompressed, voxelDataSizeDecompressed * 2)) {
		Log::error("Could not load qbt file: Failed to extract zip data of size %i, volume space: %i",
				(int)voxelDataSize, (int)voxelDataSizeDecompressed);
		if (voxelDataSize >= 4) {
			Log::debug("First 4 bytes: 0x%x 0x%x 0x%x 0x%x", voxelData[0], voxelData[1], voxelData[2], voxelData[3]);
		}
		delete [] voxelData;
		delete [] voxelDataDecompressed;
		return false;
	}
	const voxel::Region region(position, position + glm::ivec3(size) - 1);
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	uint32_t byteCounter = 0u;
	for (uint32_t x = 0; x < size.x; x++) {
		for (uint32_t z = 0; z < size.z; z++) {
			for (uint32_t y = 0; y < size.y; y++) {
				const uint32_t red   = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 0;
				const uint32_t green = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 8;
				const uint32_t blue  = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 16;
				const uint32_t alpha = ((uint32_t)255) << 24;
				const uint8_t mask   = voxelDataDecompressed[byteCounter++];
				if (mask == 0u) {
					continue;
				}
				if (_paletteSize > 0) {
					voxel::VoxelType voxelType = voxel::VoxelType::Generic;
					if (red == 0) {
						voxelType = voxel::VoxelType::Air;
					}
					const voxel::Voxel& voxel = voxel::createVoxel(voxelType, red);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				} else {
					const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
					const uint8_t index = findClosestIndex(color);
					voxel::VoxelType voxelType = voxel::VoxelType::Generic;
					if (index == 0) {
						voxelType = voxel::VoxelType::Air;
					}
					const voxel::Voxel& voxel = voxel::createVoxel(voxelType, index);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				}
			}
		}
	}
	delete [] voxelData;
	delete [] voxelDataDecompressed;
	volumes.push_back(VoxelVolume(volume, name, true, glm::ivec3(pivot)));
	return true;
}

/**
 * Model Node
 * TypeID 4 bytes, uint = 1
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this node)
 * ChildCount 4 bytes, uint, number of child nodes
 * Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadModel(io::FileStream& stream, VoxelVolumes& volumes) {
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	Log::debug("Found %u children", childCount);
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, volumes)) {
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadNode(io::FileStream& stream, VoxelVolumes& volumes) {
	uint32_t nodeTypeID;
	wrap(stream.readInt(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));
	Log::debug("Data size: %u", dataSize);

	const int before = stream.remaining();
	switch (nodeTypeID) {
	case 0: {
		Log::debug("Found matrix");
		if (!loadMatrix(stream, volumes)) {
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	}
	case 1:
		Log::debug("Found model");
		if (!loadModel(stream, volumes)) {
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case 2:
		Log::debug("Found compound");
		if (!loadCompound(stream, volumes)) {
			return false;
		}
		Log::debug("Compound of size %u loaded", dataSize);
		break;
	default:
		Log::debug("Skip unknown node type %u of size %u", nodeTypeID, dataSize);
		// skip node if unknown
		stream.skip(dataSize);
		break;
	}
	const int after = stream.remaining();
	const int delta = before - after;
	if (delta != (int)dataSize) {
		Log::error("Unexpected chunk size for type id %i, read %i, expected: %i",
				nodeTypeID, delta, (int)dataSize);
	}
	return true;
}

bool QBTFormat::loadColorMap(io::FileStream& stream) {
	uint32_t colorCount;
	wrap(stream.readInt(colorCount));
	Log::debug("Load color map with %u colors", colorCount);
	if (colorCount > 0xFFFF) {
		_paletteSize = 0;
		Log::error("Sanity check for max colors failed (%u)", colorCount);
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

bool QBTFormat::loadFromStream(io::FileStream& stream, VoxelVolumes& volumes) {
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

	Log::debug("QBT with version %i.%i", (int)versionMajor, (int)versionMinor);

	glm::vec3 globalScale;
	wrap(stream.readFloat(globalScale.x));
	wrap(stream.readFloat(globalScale.y));
	wrap(stream.readFloat(globalScale.z));

	for (int i = 0; i < 2; ++i) {
		if (stream.remaining() <= 0) {
			break;
		}
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
			if (_paletteSize == 0) {
				Log::debug("No color map found");
			} else {
				Log::debug("Color map loaded");
			}
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
			Log::debug("Unknown section found: %c%c%c%c%c%c%c%c",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
			return false;
		}
	}
	return true;
}

VoxelVolumes QBTFormat::loadGroups(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qbt file: File doesn't exist");
		return VoxelVolumes();
	}
	io::FileStream stream(file.get());
	VoxelVolumes volumes;
	if (!loadFromStream(stream, volumes)) {
		return VoxelVolumes();
	}
	return volumes;
}

#undef wrapSave
#undef wrapSaveFree
#undef wrap
#undef wrapBool

}
