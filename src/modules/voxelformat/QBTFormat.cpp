/**
 * @file
 */

#include "QBTFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include "voxel/MaterialColor.h"
#include "core/Log.h"
#include <glm/common.hpp>

namespace voxel {

static const bool MergeCompounds = true;

#define wrapSave(write) \
	if ((write) == false) { \
		Log::error("Could not save qbt file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveFree(write) \
	if ((write) == false) { \
		Log::error("Could not save qbt file: " CORE_STRINGIFY(write) " failed"); \
		delete[] compressedBuf; \
		delete[] zlibBuffer; \
		return false; \
	}

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool QBTFormat::saveMatrix(io::FileStream& stream, const VoxelVolume& volume, bool colorMap) const {
	const voxel::Region& region = volume.volume->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	const int zlibBufSize = size.x * size.y * size.z * (int)sizeof(uint32_t);
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
					*zlibBuf++ = (uint8_t)0; // mask 0 == air
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
				// mask != 0 means solid, 1 is core (surrounded by others and not visible)
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

	wrapSaveFree(stream.writeInt(0)); // node type matrix
	const size_t nameLength = volume.name.size();
	const size_t nameSize = sizeof(uint32_t) + nameLength;
	const size_t positionSize = 3 * sizeof(uint32_t);
	const size_t localScaleSize = 3 * sizeof(uint32_t);
	const size_t pivotSize = 3 * sizeof(float);
	const size_t sizeSize = 3 * sizeof(uint32_t);
	const size_t compressedDataSize = sizeof(uint32_t) + realBufSize;
	const uint32_t datasize = (uint32_t)(nameSize + positionSize + localScaleSize + pivotSize + sizeSize + compressedDataSize);
	wrapSaveFree(stream.writeInt(datasize));

	const size_t chunkStartPos = stream.pos();
	wrapSaveFree(stream.writeInt(nameLength));
	wrapSaveFree(stream.writeString(volume.name, false));
	Log::debug("Save matrix with name %s", volume.name.c_str());

	wrapSaveFree(stream.writeInt(mins.x));
	wrapSaveFree(stream.writeInt(mins.y));
	wrapSaveFree(stream.writeInt(mins.z));

	glm::uvec3 localScale { 1 };
	wrapSaveFree(stream.writeInt(localScale.x));
	wrapSaveFree(stream.writeInt(localScale.y));
	wrapSaveFree(stream.writeInt(localScale.z));

	wrapSaveFree(stream.writeFloat(volume.pivot.x));
	wrapSaveFree(stream.writeFloat(volume.pivot.y));
	wrapSaveFree(stream.writeFloat(volume.pivot.z));

	wrapSaveFree(stream.writeInt(size.x));
	wrapSaveFree(stream.writeInt(size.y));
	wrapSaveFree(stream.writeInt(size.z));

	Log::debug("save %i compressed bytes", (int)realBufSize);
	wrapSaveFree(stream.writeInt(realBufSize));
	if (stream.write(compressedBuf, realBufSize) != 0) {
		Log::error("Could not save qbt file: failed to write the compressed buffer");
		delete[] compressedBuf;
		delete[] zlibBuffer;
		return false;
	}
	const size_t chunkEndPos = stream.pos();

	delete[] compressedBuf;
	delete[] zlibBuffer;

	return (size_t)datasize == chunkEndPos - chunkStartPos;
}

bool QBTFormat::saveColorMap(io::FileStream& stream) const {
	wrapSave(stream.writeString("COLORMAP", false));
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	wrapSave(stream.writeInt((uint32_t)materialColors.size()));
	for (const glm::vec4& c : materialColors) {
		const uint32_t rgba = core::Color::getRGBA(c);
		wrapSave(stream.writeInt(rgba));
	}
	return true;
}

bool QBTFormat::saveModel(io::FileStream& stream, const VoxelVolumes& volumes, bool colorMap) const {
	int children = 0;
	for (auto& v : volumes) {
		if (v.volume == nullptr) {
			continue;
		}
		++children;
	}
	wrapSave(stream.writeInt(1)); // node type model
	if (children == 0) {
		wrapSave(stream.writeInt(sizeof(uint32_t)));
		wrapSave(stream.writeInt(0));
		return false;
	}
	const int sizePos = stream.pos();
	wrapSave(stream.writeInt(0));

	const int dataStart = stream.pos();
	wrapSave(stream.writeInt(children));

	bool success = true;
	for (auto& v : volumes) {
		if (v.volume == nullptr) {
			continue;
		}
		if (!saveMatrix(stream, v, colorMap)) {
			success = false;
		}
	}

	const int dataEnd = stream.pos();
	const int delta = dataEnd - dataStart;

	stream.seek(sizePos);
	wrapSave(stream.writeInt(delta));

	return success;
}

bool QBTFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());
	wrapSave(stream.writeInt(FourCC('Q','B',' ','2')))
	wrapSave(stream.writeByte(1));
	wrapSave(stream.writeByte(0));
	wrapSave(stream.writeFloat(1.0f));
	wrapSave(stream.writeFloat(1.0f));
	wrapSave(stream.writeFloat(1.0f));
	bool colorMap = false;
	if (colorMap) {
		if (!saveColorMap(stream)) {
			return false;
		}
	}
	int layers = 0;
	if (!stream.writeString("DATATREE", false)) {
		return false;
	}
	if (!saveModel(stream, volumes, colorMap)) {
		return false;
	}
	Log::debug("Saved %i layers", layers);
	return true;
}

bool QBTFormat::skipNode(io::SeekableReadStream& stream) {
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
bool QBTFormat::loadCompound(io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	if (!loadMatrix(stream, volumes)) {
		return false;
	}
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	Log::debug("Load %u children", childCount);
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
bool QBTFormat::loadMatrix(io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	char name[1024];
	uint32_t nameLength;
	wrap(stream.readInt(nameLength));
	if ((size_t)nameLength >= sizeof(name)) {
		Log::error("Name buffer not big enough");
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
	if (glm::any(glm::greaterThan(size, glm::uvec3(MaxRegionSize)))) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(size, glm::uvec3(1)))) {
		Log::warn("Size of matrix results in empty space");
		return false;
	}
	uint8_t* voxelData = new uint8_t[voxelDataSize];
	wrap(stream.read(voxelData, voxelDataSize));

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
	delete [] voxelData;
	const voxel::Region region(position, position + glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		delete [] voxelDataDecompressed;
		return false;
	}
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	uint32_t byteCounter = 0u;
	_colorsSize = 0;
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
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, red);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				} else {
					const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
					const uint8_t index = findClosestIndex(color);
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
					//_colors[_colorsSize++] = core::Color::getRGBA(color);
				}
			}
		}
	}
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
bool QBTFormat::loadModel(io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	if (childCount > 2048u) {
		Log::error("Max child count exceeded: %i", (int)childCount);
		return false;
	}
	Log::debug("Found %u children", childCount);
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, volumes)) {
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadNode(io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	uint32_t nodeTypeID;
	wrap(stream.readInt(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));
	Log::debug("Data size: %u", dataSize);

	switch (nodeTypeID) {
	case 0: {
		Log::debug("Found matrix");
		if (!loadMatrix(stream, volumes)) {
			Log::error("Failed to load matrix");
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	}
	case 1:
		Log::debug("Found model");
		if (!loadModel(stream, volumes)) {
			Log::error("Failed to load model");
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case 2:
		Log::debug("Found compound");
		if (!loadCompound(stream, volumes)) {
			Log::error("Failed to load compound");
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
	return true;
}

bool QBTFormat::loadColorMap(io::SeekableReadStream& stream) {
	uint32_t colorCount;
	wrap(stream.readInt(colorCount));
	Log::debug("Load color map with %u colors", colorCount);
	if (colorCount > _palette.size()) {
		_paletteSize = 0;
		Log::error("Sanity check for max colors failed (%u)", colorCount);
		return false;
	}
	_paletteSize = 0;
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
		_colors[i] = core::Color::getRGBA(color);
		const uint8_t index = findClosestIndex(color);
		_palette[i] = index;
	}
	_colorsSize = colorCount;
	_paletteSize = colorCount;
	return true;
}

bool QBTFormat::loadFromStream(io::SeekableReadStream& stream, VoxelVolumes& volumes) {
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
				Log::error("Failed to load color map");
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
				Log::error("Failed to load node");
				return false;
			}
		} else {
			Log::error("Unknown section found: %c%c%c%c%c%c%c%c",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	return loadFromStream(stream, volumes);
}

#undef wrapSave
#undef wrapSaveFree
#undef wrap
#undef wrapBool

}
