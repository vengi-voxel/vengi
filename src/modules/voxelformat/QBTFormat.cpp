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
#include "io/FileStream.h"
#include "io/BufferedZipReadStream.h"
#include "voxel/MaterialColor.h"
#include "core/Log.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/common.hpp>

namespace voxel {

namespace qbt {
static const bool MergeCompounds = true;
const int NODE_TYPE_MATRIX = 0;
const int NODE_TYPE_MODEL = 1;
const int NODE_TYPE_COMPOUND = 2;
}

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

bool QBTFormat::saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node, bool colorMap) const {
	const voxel::Region& region = node.region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	const int zlibBufSize = size.x * size.y * size.z * (int)sizeof(uint32_t);
	core_assert(zlibBufSize > 0);
	uint8_t * const zlibBuffer = new uint8_t[zlibBufSize];
	const uint32_t compressedBufSize = core::zip::compressBound(zlibBufSize);
	uint8_t *compressedBuf = new uint8_t[compressedBufSize];
	const voxel::Palette& palette = voxel::getPalette();

	uint8_t* zlibBuf = zlibBuffer;
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			for (int y = mins.y; y <= maxs.y; ++y) {
				const Voxel& voxel = node.volume()->voxel(x, y, z);
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
					const glm::u8vec4 &voxelColor = core::Color::toRGBA(palette.colors[voxel.getColor()]);
					//const uint8_t alpha = voxelColor.a * 255.0f;
					*zlibBuf++ = voxelColor.r;
					*zlibBuf++ = voxelColor.g;
					*zlibBuf++ = voxelColor.b;
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

	wrapSaveFree(stream.writeUInt32(qbt::NODE_TYPE_MATRIX)); // node type matrix
	const size_t nameLength = node.name().size();
	const size_t nameSize = sizeof(uint32_t) + nameLength;
	const size_t positionSize = 3 * sizeof(uint32_t);
	const size_t localScaleSize = 3 * sizeof(uint32_t);
	const size_t pivotSize = 3 * sizeof(float);
	const size_t sizeSize = 3 * sizeof(uint32_t);
	const size_t compressedDataSize = sizeof(uint32_t) + realBufSize;
	const uint32_t datasize = (uint32_t)(nameSize + positionSize + localScaleSize + pivotSize + sizeSize + compressedDataSize);
	wrapSaveFree(stream.writeUInt32(datasize));

	const size_t chunkStartPos = stream.pos();
	wrapSaveFree(stream.writeUInt32(nameLength));
	wrapSaveFree(stream.writeString(node.name(), false));
	Log::debug("Save matrix with name %s", node.name().c_str());

	wrapSaveFree(stream.writeUInt32(mins.x));
	wrapSaveFree(stream.writeUInt32(mins.y));
	wrapSaveFree(stream.writeUInt32(mins.z));

	glm::uvec3 localScale { 1 };
	wrapSaveFree(stream.writeUInt32(localScale.x));
	wrapSaveFree(stream.writeUInt32(localScale.y));
	wrapSaveFree(stream.writeUInt32(localScale.z));

	const glm::vec3 &pivot = node.normalizedPivot();
	wrapSaveFree(stream.writeFloat(pivot.x));
	wrapSaveFree(stream.writeFloat(pivot.y));
	wrapSaveFree(stream.writeFloat(pivot.z));

	wrapSaveFree(stream.writeUInt32(size.x));
	wrapSaveFree(stream.writeUInt32(size.y));
	wrapSaveFree(stream.writeUInt32(size.z));

	Log::debug("save %i compressed bytes", (int)realBufSize);
	wrapSaveFree(stream.writeUInt32(realBufSize));
	if (stream.write(compressedBuf, realBufSize) == -1) {
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

bool QBTFormat::saveColorMap(io::SeekableWriteStream& stream) const {
	wrapSave(stream.writeString("COLORMAP", false));
	const voxel::Palette& palette = voxel::getPalette();
	wrapSave(stream.writeUInt32(palette.colorCount));
	for (int i = 0; i < palette.colorCount; ++i) {
		wrapSave(stream.writeUInt32(palette.colors[i]));
	}
	return true;
}

bool QBTFormat::saveModel(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, bool colorMap) const {
	int children = (int)sceneGraph.size();
	wrapSave(stream.writeUInt32(qbt::NODE_TYPE_MODEL));
	if (children == 0) {
		wrapSave(stream.writeUInt32(sizeof(uint32_t)));
		wrapSave(stream.writeUInt32(0));
		return false;
	}
	const uint32_t sizePos = stream.pos();
	wrapSave(stream.writeUInt32(0));

	const uint32_t dataStart = stream.pos();
	wrapSave(stream.writeUInt32(children));

	bool success = true;
	for (const SceneGraphNode& node : sceneGraph) {
		if (!saveMatrix(stream, node, colorMap)) {
			success = false;
		}
	}

	const uint32_t dataEnd = stream.pos();
	const uint32_t delta = dataEnd - dataStart;

	stream.seek(sizePos);
	wrapSave(stream.writeUInt32(delta));

	return success;
}

bool QBTFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapSave(stream.writeUInt32(FourCC('Q','B',' ','2')))
	wrapSave(stream.writeUInt8(1));
	wrapSave(stream.writeUInt8(0));
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
	if (!saveModel(stream, sceneGraph, colorMap)) {
		return false;
	}
	Log::debug("Saved %i layers", layers);
	return true;
}

bool QBTFormat::skipNode(io::SeekableReadStream& stream) {
	// node type, can be ignored
	uint32_t nodeTypeId;
	wrap(stream.readUInt32(nodeTypeId));
	uint32_t dataSize;
	wrap(stream.readUInt32(dataSize));
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
bool QBTFormat::loadCompound(io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent) {
	SceneGraphNode node(SceneGraphNodeType::Group);
	node.setName("Compound");
	int nodeId = sceneGraph.emplace(core::move(node), parent);

	if (!loadMatrix(stream, sceneGraph, nodeId)) {
		return false;
	}
	uint32_t childCount;
	wrap(stream.readUInt32(childCount));
	Log::debug("Load %u children", childCount);
	for (uint32_t i = 0; i < childCount; ++i) {
		if (qbt::MergeCompounds) {
			// if you don't need the datatree you can skip child nodes
			if (!skipNode(stream)) {
				return false;
			}
		} else {
			if (!loadNode(stream, sceneGraph, nodeId)) {
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
bool QBTFormat::loadMatrix(io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent) {
	char name[1024];
	uint32_t nameLength;
	wrap(stream.readUInt32(nameLength));
	if ((size_t)nameLength >= sizeof(name)) {
		Log::error("Name buffer not big enough");
		return false;
	}
	wrapBool(stream.readString(nameLength, name));
	name[nameLength] = '\0';
	Log::debug("Matrix name: %s", name);
	glm::ivec3 position;
	glm::uvec3 localScale;
	glm::uvec3 size;
	// TODO: move into transform
	wrap(stream.readInt32(position.x));
	wrap(stream.readInt32(position.y));
	wrap(stream.readInt32(position.z));
	// TODO: move into transform
	wrap(stream.readUInt32(localScale.x));
	wrap(stream.readUInt32(localScale.y));
	wrap(stream.readUInt32(localScale.z));

	SceneGraphTransform transform;
	wrap(stream.readFloat(transform.normalizedPivot.x));
	wrap(stream.readFloat(transform.normalizedPivot.y));
	wrap(stream.readFloat(transform.normalizedPivot.z));
	wrap(stream.readUInt32(size.x));
	wrap(stream.readUInt32(size.y));
	wrap(stream.readUInt32(size.z));

	uint32_t voxelDataSize;
	wrap(stream.readUInt32(voxelDataSize));
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
	const uint32_t voxelDataSizeDecompressed = size.x * size.y * size.z * sizeof(uint32_t);
	io::BufferedZipReadStream zipStream(stream, voxelDataSize, voxelDataSizeDecompressed * 2);

	const voxel::Region region(position, position + glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	for (int32_t x = 0; x < (int)size.x; x++) {
		for (int32_t z = 0; z < (int)size.z; z++) {
			for (int32_t y = 0; y < (int)size.y; y++) {
				uint8_t red;
				wrap(zipStream.readUInt8(red))
				uint8_t green;
				wrap(zipStream.readUInt8(green))
				uint8_t blue;
				wrap(zipStream.readUInt8(blue))
				uint8_t mask;
				wrap(zipStream.readUInt8(mask))
				if (mask == 0u) {
					continue;
				}
				if (_palette.colorCount > 0) {
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, red);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				} else {
					const uint32_t color = core::Color::getRGBA(red, green, blue);
					const uint8_t index = voxel::getPalette().getClosestMatch(color);
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
					volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				}
			}
		}
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(name);
	node.setTransform(0, transform, true);
	const int id = sceneGraph.emplace(core::move(node), parent);
	return id != -1;
}

/**
 * Model Node
 * TypeID 4 bytes, uint = 1
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this node)
 * ChildCount 4 bytes, uint, number of child nodes
 * Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadModel(io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent) {
	uint32_t childCount;
	wrap(stream.readUInt32(childCount));
	if (childCount > 2048u) {
		Log::error("Max child count exceeded: %i", (int)childCount);
		return false;
	}
	Log::debug("Found %u children", childCount);
	SceneGraphNode node(SceneGraphNodeType::Group);
	node.setName("Model");
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, sceneGraph, nodeId)) {
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadNode(io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent) {
	uint32_t nodeTypeID;
	wrap(stream.readUInt32(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readUInt32(dataSize));
	Log::debug("Data size: %u", dataSize);

	switch (nodeTypeID) {
	case qbt::NODE_TYPE_MATRIX: {
		Log::debug("Found matrix");
		if (!loadMatrix(stream, sceneGraph, parent)) {
			Log::error("Failed to load matrix");
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	}
	case qbt::NODE_TYPE_MODEL:
		Log::debug("Found model");
		if (!loadModel(stream, sceneGraph, parent)) {
			Log::error("Failed to load model");
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case qbt::NODE_TYPE_COMPOUND:
		Log::debug("Found compound");
		if (!loadCompound(stream, sceneGraph, parent)) {
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
	wrap(stream.readUInt32(colorCount));
	Log::debug("Load color map with %u colors", colorCount);
	if (colorCount > _paletteMapping.size()) {
		Log::error("Sanity check for max colors failed (%u)", colorCount);
		return false;
	}
	_palette.colorCount = (int)colorCount;
	for (uint32_t i = 0; i < colorCount; ++i) {
		uint8_t colorByteR;
		uint8_t colorByteG;
		uint8_t colorByteB;
		uint8_t colorByteVisMask;
		wrap(stream.readUInt8(colorByteR));
		wrap(stream.readUInt8(colorByteG));
		wrap(stream.readUInt8(colorByteB));
		wrap(stream.readUInt8(colorByteVisMask));

		const uint32_t red   = ((uint32_t)colorByteR) << 24;
		const uint32_t green = ((uint32_t)colorByteG) << 16;
		const uint32_t blue  = ((uint32_t)colorByteB) << 8;
		const uint32_t alpha = ((uint32_t)255) << 0;

		const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
		_palette.colors[i] = core::Color::getRGBA(color);
		const uint8_t index = findClosestIndex(color);
		_paletteMapping[i] = index;
	}
	return true;
}

bool QBTFormat::loadFromStream(io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint32_t header;
	wrap(stream.readUInt32(header))
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
	wrap(stream.readUInt8(versionMajor))

	uint8_t versionMinor;
	wrap(stream.readUInt8(versionMinor))

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
			if (_palette.colorCount == 0) {
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
			if (!loadNode(stream, sceneGraph, sceneGraph.root().id())) {
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

bool QBTFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	return loadFromStream(stream, sceneGraph);
}

#undef wrapSave
#undef wrapSaveFree
#undef wrap
#undef wrapBool

}
