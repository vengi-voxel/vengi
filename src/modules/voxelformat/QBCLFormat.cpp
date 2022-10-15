/**
 * @file
 */

#include "QBCLFormat.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/ScopedPtr.h"
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
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/PaletteLookup.h"
#include "voxelformat/SceneGraph.h"

namespace voxelformat {

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

static bool saveColor(io::WriteStream &stream, core::RGBA color) {
	// VisibilityMask::AlphaChannelVisibleByValue
	wrapSave(stream.writeUInt8(color.r))
	wrapSave(stream.writeUInt8(color.g))
	wrapSave(stream.writeUInt8(color.b))
	wrapSave(stream.writeUInt8(color.a > 0 ? 255 : 0))
	return true;
}

static int writeRLE(io::WriteStream& stream, const voxel::Voxel& voxel, uint8_t count, const voxel::Palette &palette) {
	if (count == 0) {
		return 0;
	}
	core::RGBA color(0, 0, 0, 0);
	if (!voxel::isAir(voxel.getMaterial())) {
		color = palette.colors[voxel.getColor()];
	}
	if (count == 1) {
		wrapSaveColor(saveColor(stream, color))
		return 1;
	}

	if (count == 2) {
		wrapSaveColor(saveColor(stream, color))
		wrapSaveColor(saveColor(stream, color))
	} else if (count > 2) {
		wrapSaveColor(stream.writeUInt8(count))			 // r
		wrapSaveColor(stream.writeUInt8(0))				 // g
		wrapSaveColor(stream.writeUInt8(0))				 // b
		wrapSaveColor(stream.writeUInt8(qbcl::RLE_FLAG)) // mask
		wrapSaveColor(saveColor(stream, color))
	}
	return 2;
}

bool QBCLFormat::saveMatrix(io::SeekableWriteStream& outStream, const SceneGraphNode& node) const {
	const voxel::Region& region = node.region();
	const voxelformat::SceneGraphTransform &transform = node.transform(0);
	const glm::ivec3& translation = transform.localTranslation();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	wrapSave(outStream.writeUInt32(qbcl::NODE_TYPE_MATRIX));
	wrapSave(outStream.writeUInt32(1)) // unknown
	wrapSave(outStream.writePascalStringUInt32LE(node.name()))
	wrapSave(outStream.writeUInt8(1)) // unknown
	wrapSave(outStream.writeUInt8(1)) // unknown
	wrapSave(outStream.writeUInt8(0)) // unknown

	wrapSave(outStream.writeUInt32(size.x))
	wrapSave(outStream.writeUInt32(size.y))
	wrapSave(outStream.writeUInt32(size.z))

	wrapSave(outStream.writeInt32(translation.x))
	wrapSave(outStream.writeInt32(translation.y))
	wrapSave(outStream.writeInt32(translation.z))

	const glm::vec3 &normalizedPivot = transform.pivot();
	wrapSave(outStream.writeFloat(/* TODO mins.x +*/ normalizedPivot.x * size.x))
	wrapSave(outStream.writeFloat(/* TODO mins.y +*/ normalizedPivot.y * size.y))
	wrapSave(outStream.writeFloat(/* TODO mins.z +*/ normalizedPivot.z * size.z))

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
				const voxel::Voxel& voxel = v->voxel(x, y, z);
				const int paletteIdx = voxel.getColor();
				if (previousColor == -1) {
					previousColor = paletteIdx;
					previousVoxel = voxel;
					rleCount = 1;
				} else if (previousColor != paletteIdx || rleCount == 255) {
					rleEntries += writeRLE(rleDataStream, previousVoxel, rleCount, node.palette());
					rleCount = 1;
					previousColor = paletteIdx;
					previousVoxel = voxel;
				} else {
					++rleCount;
				}
			}
			rleEntries += writeRLE(rleDataStream, previousVoxel, rleCount, node.palette());
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
	wrapSave(stream.writePascalStringUInt32LE(sceneGraph.root().name()))
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
	wrapBool(stream.writeUInt32(131331))
	wrapBool(stream.writeUInt32(qbcl::VERSION))
	wrapBool(stream.writeUInt32(0)) // thumbnail w/h
	wrapBool(stream.writeUInt32(0)) // thumbnail w/h

	const SceneGraphNode& rootNode = sceneGraph.root();
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Title")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Description")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Metadata")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Author")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Company")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Website")))
	wrapBool(stream.writePascalStringUInt32LE(rootNode.property("Copyright")))

	uint8_t guid[16] {0}; // TODO: not yet sure about this - looks like a digest
	if (stream.write(guid, lengthof(guid)) == -1) {
		Log::error("Failed to write guid into stream");
		return false;
	}

	return saveModel(stream, sceneGraph);
}

size_t QBCLFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	Header header;
	wrapBool(readHeader(stream, header))
	header.loadPalette = true;

	SceneGraph sceneGraph;
	wrapBool(readNodes(filename, stream, sceneGraph, sceneGraph.root().id(), palette, header))

	return palette.colorCount;
}

bool QBCLFormat::readMatrix(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header) {
	SceneGraphTransform transform;
	Log::debug("Matrix name: %s", name.c_str());

	glm::uvec3 size;
	wrap(stream.readUInt32(size.x));
	wrap(stream.readUInt32(size.y));
	wrap(stream.readUInt32(size.z));

	glm::ivec3 translation;
	wrap(stream.readInt32(translation.x));
	wrap(stream.readInt32(translation.y));
	wrap(stream.readInt32(translation.z));
	transform.setLocalTranslation(translation);

	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x));
	wrap(stream.readFloat(pivot.y));
	wrap(stream.readFloat(pivot.z));

	// the pivot is given in voxel coordinates
	//transform.setPivot(pivot / glm::vec3(size));

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

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}

	io::ZipReadStream zipStream(stream, (int)compressedDataSize);
	core::ScopedPtr<voxel::RawVolume> volume(new voxel::RawVolume(region));
	uint32_t index = 0;

	voxel::PaletteLookup palLookup(palette);

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
					const core::RGBA color = core::Color::getRGBA(red, green, blue, alpha);
					if (header.loadPalette) {
						palette.addColorToPalette(color);
					} else {
						const uint8_t palIndex = palLookup.findClosestIndex(color);
						const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIndex);
						const uint32_t x = (index / size.z);
						const uint32_t z = index % size.z;
						for (int j = 0; j < rleLength; ++j) {
							volume->setVoxel((int)x, y, (int)z, voxel);
							++y;
						}
					}
				}
				// we've read another color value for the rle values
				++i;
			} else if (mask == 0) {
				++y;
			} else {
				// Uncompressed
				const core::RGBA color = core::Color::getRGBA(red, green, blue);
				if (header.loadPalette) {
					palette.addColorToPalette(color);
				} else {
					const uint32_t x = (index / size.z);
					const uint32_t z = index % size.z;
					const uint8_t palIndex = palLookup.findClosestIndex(color);
					const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIndex);
					volume->setVoxel((int)x, y, (int)z, voxel);
				}
				++y;
			}
		}

		index++;
	}

	if (header.loadPalette) {
		return true;
	}

	SceneGraphNode node;
	node.setVolume(volume.release(), true);

	node.setPalette(palLookup.palette());
	if (name.empty()) {
		node.setName("Matrix");
	} else {
		node.setName(name);
	}
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	const int id = sceneGraph.emplace(core::move(node), parent);
	return id != -1;
}

bool QBCLFormat::readModel(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header) {
	const size_t skip = 3 * 3 * sizeof(float);
	stream.skip((int64_t)skip); // TODO: rotation matrix?
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
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId, palette, header))
	}
	return true;
}

bool QBCLFormat::readCompound(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, const core::String &name, voxel::Palette &palette, Header &header) {
	SceneGraphNode node(SceneGraphNodeType::Group);
	if (name.empty()) {
		node.setName("Compound");
	} else {
		node.setName(name);
	}
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	wrapBool(readMatrix(filename, stream, sceneGraph, nodeId, name, palette, header))
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	Log::debug("Found %u children in compound '%s'", childCount, name.c_str());
	for (uint32_t i = 0; i < childCount; ++i) {
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId, palette, header))
	}
	return true;
}

bool QBCLFormat::readNodes(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int parent, voxel::Palette &palette, Header &header) {
	uint32_t type;
	wrap(stream.readUInt32(type))
	uint32_t dataSize;
	wrap(stream.readUInt32(dataSize));
	Log::debug("Data size: %u", dataSize);

	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	stream.skip(3); // ColorFormat, ZAxisOrientation, Compression? (see QBFormat)
	switch (type) {
	case qbcl::NODE_TYPE_MATRIX:
		Log::debug("Found matrix");
		if (!readMatrix(filename, stream, sceneGraph, parent, name, palette, header)) {
			Log::error("Failed to load matrix %s", name.c_str());
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	case qbcl::NODE_TYPE_MODEL:
		Log::debug("Found model");
		if (!readModel(filename, stream, sceneGraph, parent, name, palette, header)) {
			Log::error("Failed to load model %s", name.c_str());
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case qbcl::NODE_TYPE_COMPOUND:
		Log::debug("Found compound");
		if (!readCompound(filename, stream, sceneGraph, parent, name, palette, header)) {
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

bool QBCLFormat::readHeader(io::SeekableReadStream& stream, Header &header) {
	wrap(stream.readUInt32(header.magic))
	if (header.magic != FourCC('Q', 'B', 'C', 'L')) {
		Log::error("Invalid magic found - no qbcl file");
		return false;
	}
	wrap(stream.readUInt32(header.version))
	wrap(stream.readUInt32(header.fileVersion))
	if (header.fileVersion != qbcl::VERSION) {
		Log::error("Unknown version found: %i", header.fileVersion);
		return false;
	}
	wrap(stream.readUInt32(header.thumbWidth))
	wrap(stream.readUInt32(header.thumbHeight))
	if (stream.skip(((int64_t)header.thumbWidth * (int64_t)header.thumbHeight * 4)) == -1) {
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read));
		return false;
	}

	wrapBool(stream.readPascalStringUInt32LE(header.title))
	wrapBool(stream.readPascalStringUInt32LE(header.desc))
	wrapBool(stream.readPascalStringUInt32LE(header.metadata))
	wrapBool(stream.readPascalStringUInt32LE(header.author))
	wrapBool(stream.readPascalStringUInt32LE(header.company))
	wrapBool(stream.readPascalStringUInt32LE(header.website))
	wrapBool(stream.readPascalStringUInt32LE(header.copyright))

	if (stream.read(header.guid, lengthof(header.guid)) == -1) {
		Log::error("Failed to read the guid");
		return false;
	}
	return true;
}

bool QBCLFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) {
	Header header;
	wrapBool(readHeader(stream, header))

	SceneGraphNode& rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty("Title", header.title);
	rootNode.setProperty("Description", header.desc);
	rootNode.setProperty("Metadata", header.metadata);
	rootNode.setProperty("Author", header.author);
	rootNode.setProperty("Company", header.company);
	rootNode.setProperty("Website", header.website);
	rootNode.setProperty("Copyright", header.copyright);

	voxel::Palette palCopy = palette;
	wrapBool(readNodes(filename, stream, sceneGraph, rootNode.id(), palCopy, header))

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
	if (!img->loadRGBA(buf, (int)thumbWidth, (int)thumbHeight)) {
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
