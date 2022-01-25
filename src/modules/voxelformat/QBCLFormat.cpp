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
#include "io/BufferedZipReadStream.h"

namespace voxel {

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

bool QBCLFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
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
	transform.position = position;

	wrap(stream.readFloat(transform.normalizedPivot.x));
	wrap(stream.readFloat(transform.normalizedPivot.y));
	wrap(stream.readFloat(transform.normalizedPivot.z));

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
	io::BufferedZipReadStream zipStream(stream, voxelDataSize, voxelDataSizeDecompressed);

	const voxel::Region region(position, position + glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	_colorsSize = 0;
	uint32_t index = 0;

	while (!zipStream.eos()) {
		int y = 0;
		uint16_t dataSize;
		wrap(zipStream.readUInt16(dataSize))
		for (int i = 0; i < dataSize; i++) {
			uint8_t red;
			wrap(zipStream.readUInt8(red))
			uint8_t green;
			wrap(zipStream.readUInt8(green))
			uint8_t blue;
			wrap(zipStream.readUInt8(blue))
			uint8_t mask;
			wrap(zipStream.readUInt8(mask))

			if (mask == 2) {
				// RLE
				uint8_t rleLength = red;
				wrap(zipStream.readUInt8(red))
				wrap(zipStream.readUInt8(green))
				wrap(zipStream.readUInt8(blue))
				uint8_t alpha;
				wrap(zipStream.readUInt8(alpha))
				if (mask == 0 || alpha == 0) {
					y += rleLength;
				} else {
					for (int j = 0; j < rleLength; j++) {
						uint32_t x = (index / size.z);
						uint32_t z = index % size.z;

						const glm::vec4& color = core::Color::fromRGBA(red, green, blue, 255);
						const uint8_t index = findClosestIndex(color);
						const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
						volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
						y++;
					}
				}
				++i;
			} else if (mask == 0) {
				++y;
			} else {
				// Uncompressed
				uint32_t x = (index / size.z);
				uint32_t z = index % size.z;
				const glm::vec4& color = core::Color::fromRGBA(red, green, blue, 255);
				const uint8_t index = findClosestIndex(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				volume->setVoxel(position.x + x, position.y + y, position.z + z, voxel);
				y++;
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
	node.setTransform(transform, true);
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
	case 0:
		Log::debug("Found matrix");
		if (!readMatrix(filename, stream, sceneGraph, parent, name)) {
			Log::error("Failed to load matrix %s", name.c_str());
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	case 1:
		Log::debug("Found model");
		if (!readModel(filename, stream, sceneGraph, parent, name)) {
			Log::error("Failed to load model %s", name.c_str());
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case 2:
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
	if (fileVersion != 2) {
		Log::error("Unknown version found: %i", fileVersion);
		return false;
	}
	uint32_t thumbWidth;
	wrap(stream.readUInt32(thumbWidth))
	uint32_t thumbHeight;
	wrap(stream.readUInt32(thumbHeight))
	if (stream.skip((int64_t)(thumbWidth * thumbHeight * 4)) == -1) {
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
	wrap(stream.read(guid, lengthof(guid)))

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
