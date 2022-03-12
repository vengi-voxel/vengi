/**
 * @file
 */

#include "QBCLFormat.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/BufferedZipReadStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

namespace qbcl {
const int RLE_FLAG = 2;

const int VERSION = 2;

const int NODE_TYPE_MATRIX = 0;
const int NODE_TYPE_MODEL = 1;
const int NODE_TYPE_COMPOUND = 2;

}

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapImg(read) \
	if ((read) != 0) { \
		Log::error("Could not load qbcl screenshot file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return image::ImagePtr(); \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapSave(write) \
	if ((write) == false) { \
		Log::error("Could not save qbcl file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveColor(write) \
	if ((write) == false) { \
		Log::error("Could not save qbcl file: " CORE_STRINGIFY(write) " failed"); \
		return -1; \
	}

#define wrapSaveNegative(write) \
	if ((write) == -1) { \
		Log::error("Could not save qbcl file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

static bool writeString(io::SeekableWriteStream& stream, const core::String& str) {
	wrapSave(stream.writeUInt32(str.size()))
	wrapSave(stream.writeString(str, false))
	return true;
}

static int writeRLE(io::WriteStream& stream, const voxel::Voxel& voxel, uint8_t count) {
	if (count == 0) {
		return 0;
	}
	glm::u8vec4 color(0);
	if (!voxel::isAir(voxel.getMaterial())) {
		const voxel::Palette& palette = voxel::getPalette();
		color = core::Color::toRGBA(palette.colors[voxel.getColor()]);
	}
	if (count == 1) {
		wrapSaveColor(stream.writeUInt8(color.r))
		wrapSaveColor(stream.writeUInt8(color.g))
		wrapSaveColor(stream.writeUInt8(color.b))
		wrapSaveColor(stream.writeUInt8(color.a))
		return 1;
	}

	if (count == 2) {
		wrapSaveColor(stream.writeUInt8(color.r))
		wrapSaveColor(stream.writeUInt8(color.g))
		wrapSaveColor(stream.writeUInt8(color.b))
		wrapSaveColor(stream.writeUInt8(color.a))

		wrapSaveColor(stream.writeUInt8(color.r))
		wrapSaveColor(stream.writeUInt8(color.g))
		wrapSaveColor(stream.writeUInt8(color.b))
		wrapSaveColor(stream.writeUInt8(color.a))
	} else if (count > 2) {
		wrapSaveColor(stream.writeUInt8(count))			 // r
		wrapSaveColor(stream.writeUInt8(0))				 // g
		wrapSaveColor(stream.writeUInt8(0))				 // b
		wrapSaveColor(stream.writeUInt8(qbcl::RLE_FLAG)) // mask

		wrapSaveColor(stream.writeUInt8(color.r))
		wrapSaveColor(stream.writeUInt8(color.g))
		wrapSaveColor(stream.writeUInt8(color.b))
		wrapSaveColor(stream.writeUInt8(color.a))
	}
	return 2;
}

bool QBCLFormat::saveMatrix(io::SeekableWriteStream& outStream, const SceneGraphNode& node) const {
	const voxel::Region& region = node.region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	wrapSave(outStream.writeUInt32(qbcl::NODE_TYPE_MATRIX));
	wrapSave(outStream.writeUInt32(1)) // unknown
	wrapSave(writeString(outStream, node.name()))
	wrapSave(outStream.writeUInt8(1)) // unknown
	wrapSave(outStream.writeUInt8(1)) // unknown
	wrapSave(outStream.writeUInt8(0)) // unknown

	wrapSave(outStream.writeUInt32(size.x))
	wrapSave(outStream.writeUInt32(size.y))
	wrapSave(outStream.writeUInt32(size.z))

	wrapSave(outStream.writeInt32(mins.x))
	wrapSave(outStream.writeInt32(mins.y))
	wrapSave(outStream.writeInt32(mins.z))

	const glm::vec3 &pivot = node.normalizedPivot();
	wrapSave(outStream.writeFloat(pivot.x))
	wrapSave(outStream.writeFloat(pivot.y))
	wrapSave(outStream.writeFloat(pivot.z))

	uint32_t voxelDataSizePos = outStream.pos();
	wrapSave(outStream.writeUInt32(0));

	io::BufferedReadWriteStream rleDataStream(size.x * size.y * size.z);

	const voxel::RawVolume *v = node.volume();
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			int previousColor = -1;
			uint16_t rleEntries = 0;
			uint8_t rleCount = 0;
			voxel::Voxel previousVoxel;

			// remember the position in the stream because we have
			// to write the real value after the z loop
			const int64_t dataSizePos = rleDataStream.pos();
			wrapSave(rleDataStream.writeUInt16(rleEntries))
			for (int y = mins.y; y <= maxs.y; ++y) {
				const Voxel& voxel = v->voxel(x, y, z);
				const int paletteIdx = voxel.getColor();
				if (previousColor == -1) {
					previousColor = paletteIdx;
					previousVoxel = voxel;
					rleCount = 1;
				} else if (previousColor != paletteIdx || rleCount == 255) {
					rleEntries += writeRLE(rleDataStream, previousVoxel, rleCount);
					rleCount = 1;
					previousColor = paletteIdx;
					previousVoxel = voxel;
				} else {
					++rleCount;
				}
			}
			rleEntries += writeRLE(rleDataStream, previousVoxel, rleCount);
			wrapSaveNegative(rleDataStream.seek(dataSizePos))
			wrapSave(rleDataStream.writeUInt16(rleEntries))
			wrapSaveNegative(rleDataStream.seek(0, SEEK_END))
		}
	}


	io::ZipWriteStream zipStream(outStream);
	if (zipStream.write(rleDataStream.getBuffer(), rleDataStream.size()) == -1) {
		Log::error("Could not write compressed data");
		return false;
	}
	wrapSave(zipStream.flush())
	const int64_t compressedDataSize = zipStream.size();
	wrapSaveNegative(outStream.seek(voxelDataSizePos))
	wrapSave(outStream.writeUInt32(compressedDataSize))
	wrapSaveNegative(outStream.seek(0, SEEK_END))

	return true;
}

bool QBCLFormat::saveModel(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) const {
	int children = (int)sceneGraph.size();
	wrapSave(stream.writeUInt32(qbcl::NODE_TYPE_MODEL))
	wrapSave(stream.writeUInt32(1)) // unknown
	wrapSave(writeString(stream, sceneGraph.root().name()))
	wrapSave(stream.writeUInt8(1)) // unknown
	wrapSave(stream.writeUInt8(1)) // unknown
	wrapSave(stream.writeUInt8(0)) // unknown
	const uint8_t array[36] = {
		0x01, 0x00, 0x00,
		0x00, 0x01, 0x00,
		0x00, 0x00, 0x01,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00 };
	if (stream.write(array, sizeof(array)) == -1) {
		Log::error("Failed to write array into stream");
		return false;
	}
	wrapSave(stream.writeUInt32(children));

	bool success = true;
	for (const SceneGraphNode& node : sceneGraph) {
		if (!saveMatrix(stream, node)) {
			success = false;
		}
	}

	return success;
}

bool QBCLFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeUInt32(FourCC('Q','B','C','L')))
	wrapBool(stream.writeUInt32(257))
	wrapBool(stream.writeUInt32(qbcl::VERSION))
	wrapBool(stream.writeUInt32(0)) // thumbnail w/h
	wrapBool(stream.writeUInt32(0)) // thumbnail w/h

	wrapBool(writeString(stream, "")) // no title
	wrapBool(writeString(stream, "")) // no desc
	wrapBool(writeString(stream, "")) // no metadata
	wrapBool(writeString(stream, "")) // no author
	wrapBool(writeString(stream, "")) // no company
	wrapBool(writeString(stream, "")) // no website
	wrapBool(writeString(stream, "")) // no copyright

	uint8_t guid[16] {0};
	if (stream.write(guid, lengthof(guid)) == -1) {
		Log::error("Failed to write guid into stream");
		return false;
	}

	return saveModel(stream, sceneGraph);
}

static bool readString(io::SeekableReadStream& stream, core::String& str) {
	uint32_t length;
	wrap(stream.readUInt32(length))
	char *name = (char *)core_malloc(length + 1);
	wrapBool(stream.readString(length, name))
	name[length] = '\0';
	str = name;
	core_free(name);
	return true;
}

bool QBCLFormat::readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name) {
	SceneGraphTransform transform;
	Log::debug("Matrix name: %s", name.c_str());

	glm::uvec3 size;
	wrap(stream.readUInt32(size.x));
	wrap(stream.readUInt32(size.y));
	wrap(stream.readUInt32(size.z));

	glm::ivec3 position;
	wrap(stream.readInt32(position.x));
	wrap(stream.readInt32(position.y));
	wrap(stream.readInt32(position.z));
	//transform.position = position;

	wrap(stream.readFloat(transform.normalizedPivot.x));
	wrap(stream.readFloat(transform.normalizedPivot.y));
	wrap(stream.readFloat(transform.normalizedPivot.z));

	uint32_t compressedDataSize;
	wrap(stream.readUInt32(compressedDataSize));
	Log::debug("Matrix size: %u:%u:%u with %u bytes", size.x, size.y, size.z, compressedDataSize);
	if (compressedDataSize == 0) {
		Log::warn("Empty voxel chunk found");
		return false;
	}
	if (compressedDataSize > 0xFFFFFF) {
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

	const voxel::Region region(position, position + glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}

	io::ZipReadStream zipStream(stream, (int)compressedDataSize);
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	uint32_t index = 0;

	const voxel::Palette &palette = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);

	while (!zipStream.eos()) {
		int y = 0;
		uint16_t rleEntries;
		wrap(zipStream.readUInt16(rleEntries))
		for (int i = 0; i < (int)rleEntries; i++) {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
			uint8_t mask;

			wrap(zipStream.readUInt8(red))
			wrap(zipStream.readUInt8(green))
			wrap(zipStream.readUInt8(blue))
			wrap(zipStream.readUInt8(mask))

			if (mask == qbcl::RLE_FLAG) {
				uint8_t rleLength = red;
				uint8_t alpha;
				wrap(zipStream.readUInt8(red))
				wrap(zipStream.readUInt8(green))
				wrap(zipStream.readUInt8(blue))
				wrap(zipStream.readUInt8(alpha))

				if (alpha == 0) {
					y += rleLength;
				} else {
					const uint32_t color = core::Color::getRGBA(red, green, blue, alpha);
					const uint8_t palIndex = palette.getClosestMatch(color);
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIndex);
					for (int j = 0; j < rleLength; ++j) {
						const uint32_t x = (index / size.z);
						const uint32_t z = index % size.z;
						volume->setVoxel(position.x + (int)x, position.y + y, position.z + (int)z, voxel);
						++y;
					}
				}
				// we've read another color value for the rle values
				++i;
			} else if (mask == 0) {
				++y;
			} else {
				// Uncompressed
				const uint32_t x = (index / size.z);
				const uint32_t z = index % size.z;
				const uint32_t color = core::Color::getRGBA(red, green, blue);
				const uint8_t palIndex = palette.getClosestMatch(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIndex);
				volume->setVoxel(position.x + (int)x, position.y + y, position.z + (int)z, voxel);
				++y;
			}
		}

		index++;
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	if (name.empty()) {
		node.setName("Matrix");
	} else {
		node.setName(name);
	}
	node.setTransform(0, transform, true);
	const int id = sceneGraph.emplace(core::move(node), parent);
	return id != -1;
}

bool QBCLFormat::readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name) {
	stream.skip(36); // rotation matrix?
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	SceneGraphNode node(SceneGraphNodeType::Group);
	if (name.empty()) {
		node.setName("Model");
	} else {
		node.setName(name);
	}
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	Log::debug("Found %u children in model '%s'", childCount, name.c_str());
	for (uint32_t i = 0; i < childCount; ++i) {
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId))
	}
	return true;
}

bool QBCLFormat::readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name) {
	SceneGraphNode node(SceneGraphNodeType::Group);
	if (name.empty()) {
		node.setName("Compound");
	} else {
		node.setName(name);
	}
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	wrapBool(readMatrix(filename, stream, sceneGraph, nodeId, name))
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	Log::debug("Found %u children in compound '%s'", childCount, name.c_str());
	for (uint32_t i = 0; i < childCount; ++i) {
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId))
	}
	return true;
}

bool QBCLFormat::readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent) {
	uint32_t type;
	wrap(stream.readUInt32(type))
	uint32_t dataSize;
	wrap(stream.readUInt32(dataSize));
	Log::debug("Data size: %u", dataSize);

	core::String name;
	wrapBool(readString(stream, name))
	stream.skip(3); // ColorFormat, ZAxisOrientation, Compression? (see QBFormat)
	switch (type) {
	case qbcl::NODE_TYPE_MATRIX:
		Log::debug("Found matrix");
		if (!readMatrix(filename, stream, sceneGraph, parent, name)) {
			Log::error("Failed to load matrix %s", name.c_str());
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	case qbcl::NODE_TYPE_MODEL:
		Log::debug("Found model");
		if (!readModel(filename, stream, sceneGraph, parent, name)) {
			Log::error("Failed to load model %s", name.c_str());
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case qbcl::NODE_TYPE_COMPOUND:
		Log::debug("Found compound");
		if (!readCompound(filename, stream, sceneGraph, parent, name)) {
			Log::error("Failed to load compound %s", name.c_str());
			return false;
		}
		Log::debug("Compound of size %u loaded", dataSize);
		break;
	default:
		Log::warn("Unknown type found: '%s'", name.c_str());
		return false;
	}
	return true;
}

bool QBCLFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('Q', 'B', 'C', 'L')) {
		Log::error("Invalid magic found - no qbcl file");
		return false;
	}
	uint32_t version; // (major, minor, release, build)
	wrapImg(stream.readUInt32(version))
	uint32_t fileVersion;
	wrap(stream.readUInt32(fileVersion))
	if (fileVersion != qbcl::VERSION) {
		Log::error("Unknown version found: %i", fileVersion);
		return false;
	}
	uint32_t thumbWidth;
	wrap(stream.readUInt32(thumbWidth))
	uint32_t thumbHeight;
	wrap(stream.readUInt32(thumbHeight))
	if (stream.skip(((int64_t)thumbWidth * (int64_t)thumbHeight * 4)) == -1) {
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read));
		return false;
	}

	core::String title;
	wrapBool(readString(stream, title))
	core::String desc;
	wrapBool(readString(stream, desc))
	core::String metadata;
	wrapBool(readString(stream, metadata))
	core::String author;
	wrapBool(readString(stream, author))
	core::String company;
	wrapBool(readString(stream, company))
	core::String website;
	wrapBool(readString(stream, website))
	core::String copyright;
	wrapBool(readString(stream, copyright))

	uint8_t guid[16];
	if (stream.read(guid, lengthof(guid)) == -1) {
		Log::error("Failed to read the guid");
		return false;
	}

	SceneGraphNode& rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty("Title", title);
	rootNode.setProperty("Description", desc);
	rootNode.setProperty("Metadata", metadata);
	rootNode.setProperty("Author", author);
	rootNode.setProperty("Company", company);
	rootNode.setProperty("Website", website);
	rootNode.setProperty("Copyright", copyright);

	wrapBool(readNodes(filename, stream, sceneGraph, rootNode.id()))

	return true;
}

image::ImagePtr QBCLFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	uint32_t magic;
	wrapImg(stream.readUInt32(magic))
	if (magic != FourCC('Q', 'B', 'C', 'L')) {
		Log::error("Invalid magic found - no qbcl file");
		return image::ImagePtr();
	}
	uint32_t version;
	wrapImg(stream.readUInt32(version))
	uint32_t flags;
	wrapImg(stream.readUInt32(flags))
	uint32_t thumbWidth;
	wrapImg(stream.readUInt32(thumbWidth))
	uint32_t thumbHeight;
	wrapImg(stream.readUInt32(thumbHeight))
	image::ImagePtr img = image::createEmptyImage(filename);
	const uint32_t thumbnailSize = thumbWidth * thumbHeight * 4;

	uint8_t* buf = new uint8_t[thumbnailSize];
	if (stream.read(buf, thumbnailSize) != 0) {
		Log::error("Failed to read the qbcl thumbnail buffer of width %u and height %u", thumbWidth, thumbHeight);
		delete [] buf;
		return image::ImagePtr();
	}
	if (!img->loadRGBA(buf, (int)thumbnailSize, (int)thumbWidth, (int)thumbHeight)) {
		Log::error("Failed to load rgba image buffer of width %u and height %u", thumbWidth, thumbHeight);
		delete [] buf;
		return image::ImagePtr();
	}
	delete [] buf;
	return img;
}

}

#undef wrapImg
#undef wrap
#undef wrapBool
#undef wrapSaveColor
#undef wrapSaveNegative
#undef wrapSave
