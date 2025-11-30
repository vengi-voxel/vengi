/**
 * @file
 */

#include "QBCLFormat.h"
#include "core/Assert.h"
#include "color/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxelformat {

namespace qbcl {
const int RLE_FLAG = 2;

const int VERSION = 2;

const int NODE_TYPE_MATRIX = 0;
const int NODE_TYPE_MODEL = 1;
const int NODE_TYPE_COMPOUND = 2;

class ScopedQBCLHeader {
private:
	bool _success = true;

public:
	ScopedQBCLHeader(io::SeekableWriteStream &stream, uint32_t nodeType) {
		Log::debug("Write node type %u", nodeType);
		if (!stream.writeUInt32(nodeType)) {
			Log::error("Failed to write the node type %u", nodeType);
			_success = false;
		}
	}

	ScopedQBCLHeader(io::SeekableWriteStream &stream, scenegraph::SceneGraphNodeType type) {
		uint32_t nodeType = 0;
		switch (type) {
		case scenegraph::SceneGraphNodeType::Group:
		case scenegraph::SceneGraphNodeType::Root:
			nodeType = qbcl::NODE_TYPE_MODEL;
			Log::debug("Write model node");
			break;
		case scenegraph::SceneGraphNodeType::Model:
		case scenegraph::SceneGraphNodeType::ModelReference:
			nodeType = qbcl::NODE_TYPE_MATRIX;
			Log::debug("Write matrix node");
			break;
		default:
			Log::error("Failed to determine the node type for %u", (uint32_t)type);
			_success = false;
			break;
		}

		if (!stream.writeUInt32(nodeType)) {
			Log::error("Failed to write the node type %u", nodeType);
			_success = false;
		}
	}

	inline bool success() const {
		return _success;
	}
};

} // namespace qbcl

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

#define wrapImg(read)                                                                                                  \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load qbcl screenshot file: Not enough data in stream " CORE_STRINGIFY(read));            \
		return image::ImagePtr();                                                                                      \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

#define wrapSave(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save qbcl file: " CORE_STRINGIFY(write) " failed");                                      \
		return false;                                                                                                  \
	}

#define wrapSaveNegative(write)                                                                                        \
	if ((write) == -1) {                                                                                               \
		Log::error("Could not save qbcl file: " CORE_STRINGIFY(write) " failed");                                      \
		return false;                                                                                                  \
	}

static bool saveColor(io::WriteStream &stream, color::RGBA color) {
	// VisibilityMask::AlphaChannelVisibleByValue
	wrapSave(stream.writeUInt8(color.r))
	wrapSave(stream.writeUInt8(color.g))
	wrapSave(stream.writeUInt8(color.b))
	wrapSave(stream.writeUInt8(color.a > 0 ? 255 : 0))
	return true;
}

static bool writeRLE(io::WriteStream &stream, color::RGBA color, uint8_t count) {
	if (count == 0) {
		return true;
	}

	if (count == 1) {
		wrapSave(saveColor(stream, color))
	} else if (count == 2) {
		wrapSave(saveColor(stream, color))
		wrapSave(saveColor(stream, color))
	} else if (count > 2) {
		wrapSave(stream.writeUInt8(count))			// r
		wrapSave(stream.writeUInt8(0))				// g
		wrapSave(stream.writeUInt8(0))				// b
		wrapSave(stream.writeUInt8(qbcl::RLE_FLAG)) // mask
		wrapSave(saveColor(stream, color))
	}
	return true;
}

bool QBCLFormat::saveMatrix(io::SeekableWriteStream &outStream, const scenegraph::SceneGraph &sceneGraph,
							const scenegraph::SceneGraphNode &node) const {
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	const scenegraph::SceneGraphTransform &transform = node.transform(0);
	const glm::ivec3 &translation = transform.localTranslation();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	wrapSave(outStream.writeUInt32(1)) // unknown
	wrapSave(outStream.writePascalStringUInt32LE(node.name()))
	wrapSave(outStream.writeBool(node.visible()))
	wrapSave(outStream.writeBool(true)) // unknown
	wrapSave(outStream.writeBool(node.locked()))

	wrapSave(outStream.writeUInt32(size.x))
	wrapSave(outStream.writeUInt32(size.y))
	wrapSave(outStream.writeUInt32(size.z))

	wrapSave(outStream.writeInt32(translation.x))
	wrapSave(outStream.writeInt32(translation.y))
	wrapSave(outStream.writeInt32(translation.z))

	const glm::vec3 &normalizedPivot = node.pivot();
	wrapSave(outStream.writeFloat(/* TODO: VOXELFORMAT: mins.x +*/ normalizedPivot.x * size.x))
	wrapSave(outStream.writeFloat(/* TODO: VOXELFORMAT: mins.y +*/ normalizedPivot.y * size.y))
	wrapSave(outStream.writeFloat(/* TODO: VOXELFORMAT: mins.z +*/ normalizedPivot.z * size.z))

	constexpr voxel::Voxel Empty;

	uint32_t voxelDataSizePos = outStream.pos();
	wrapSave(outStream.writeUInt32(0));

	io::BufferedReadWriteStream rleDataStream(size.x * size.y * size.z * 32);

	const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
	const palette::Palette &palette = node.palette();
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			color::RGBA currentColor;
			uint16_t rleEntries = 0;
			uint8_t count = 0;

			// remember the position in the stream because we have
			// to write the real value after the z loop
			const int64_t dataSizePos = rleDataStream.pos();
			wrapSave(rleDataStream.writeUInt16(rleEntries))
			for (int y = mins.y; y <= maxs.y; ++y) {
				const voxel::Voxel &voxel = v->voxel(x, y, z);
				color::RGBA newColor;
				if (voxel == Empty) {
					newColor = 0;
					Log::trace("Save empty voxel: x %i, y %i, z %i", x, y, z);
				} else {
					newColor = palette.color(voxel.getColor());
					Log::trace("Save voxel: x %i, y %i, z %i (color: index(%i) => rgba(%i:%i:%i:%i))", x, y, z,
							   (int)voxel.getColor(), (int)newColor.r, (int)newColor.g, (int)newColor.b,
							   (int)newColor.a);
				}
				if (newColor != currentColor || count == 255) {
					wrapSave(writeRLE(rleDataStream, currentColor, count))
					rleEntries += core_min(count, 2);
					count = 0;
					currentColor = newColor;
				}
				++count;
			}
			wrapSave(writeRLE(rleDataStream, currentColor, count))
			rleEntries += core_min(count, 2);

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

bool QBCLFormat::saveCompound(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
							  const scenegraph::SceneGraphNode &node) const {
	wrapSave(saveMatrix(stream, sceneGraph, node))
	wrapSave(stream.writeUInt32((int)node.children().size()));
	for (int nodeId : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
		wrapSave(saveNode(stream, sceneGraph, cnode))
	}
	return true;
}

bool QBCLFormat::saveModel(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						   const scenegraph::SceneGraphNode &node) const {
	const uint32_t children = (uint32_t)node.children().size();
	qbcl::ScopedQBCLHeader header(stream, node.type());
	wrapSave(stream.writeUInt32(1)) // unknown
	wrapSave(stream.writePascalStringUInt32LE(node.name()))
	wrapSave(stream.writeBool(node.visible()))
	wrapSave(stream.writeBool(true)) // unknown
	wrapSave(stream.writeBool(node.locked()))
	const glm::mat3x3 &mat = sceneGraph.transformForFrame(node, 0).worldMatrix();
	for (int col = 0; col < 3; ++col) {
		for (int row = 0; row < 3; ++row) {
			wrapSave(stream.writeFloat(mat[col][row]))
		}
	}
	wrapSave(stream.writeUInt32(children));

	for (int nodeId : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
		wrapSave(saveNode(stream, sceneGraph, cnode))
	}

	return true;
}

bool QBCLFormat::saveNode(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						  const scenegraph::SceneGraphNode &node) const {
	const scenegraph::SceneGraphNodeType type = node.type();
	if (node.isAnyModelNode()) {
		if (node.children().empty()) {
			qbcl::ScopedQBCLHeader header(stream, node.type());
			wrapSave(saveMatrix(stream, sceneGraph, node) && header.success())
		} else {
			qbcl::ScopedQBCLHeader scoped(stream, qbcl::NODE_TYPE_COMPOUND);
			wrapSave(saveCompound(stream, sceneGraph, node) && scoped.success())
		}
	} else if (type == scenegraph::SceneGraphNodeType::Group || type == scenegraph::SceneGraphNodeType::Root) {
		wrapSave(saveModel(stream, sceneGraph, node))
	}
	return true;
}

bool QBCLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &savectx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapSave(stream->writeUInt32(FourCC('Q', 'B', 'C', 'L')))
	wrapSave(stream->writeUInt32(131331))
	wrapSave(stream->writeUInt32(qbcl::VERSION))
	bool imageAdded = false;
	ThumbnailContext ctx;
	ctx.outputSize = glm::ivec2(128);
	const image::ImagePtr &image = createThumbnail(sceneGraph, savectx.thumbnailCreator, ctx);
	if (image && image->isLoaded()) {
		const int size = image->width() * image->height() * image->components();
		if (size > 0) {
			wrapSave(stream->writeUInt32(image->width()))
			wrapSave(stream->writeUInt32(image->height()))
			for (int x = 0; x < image->width(); ++x) {
				for (int y = 0; y < image->height(); ++y) {
					const color::RGBA color = image->colorAt(x, y);
					stream->writeUInt8(color.b);
					stream->writeUInt8(color.g);
					stream->writeUInt8(color.r);
					stream->writeUInt8(color.a);
				}
			}
			imageAdded = true;
		} else {
			Log::debug("Loaded image has zero size");
		}
	}

	if (!imageAdded) {
		wrapSave(stream->writeUInt32(0)) // thumbnail w/h
		wrapSave(stream->writeUInt32(0)) // thumbnail w/h
	}

	const scenegraph::SceneGraphNode &rootNode = sceneGraph.root();
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropTitle)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropDescription)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropMetadata)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropAuthor)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropCompany)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropWebsite)))
	wrapSave(stream->writePascalStringUInt32LE(rootNode.property(scenegraph::PropCopyright)))
	wrapSave(stream->writeUInt64(0)) // timestamp1
	wrapSave(stream->writeUInt64(0)) // timestamp2
	return saveNode(*stream, sceneGraph, sceneGraph.root());
}

size_t QBCLFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	Header header;
	wrapBool(readHeader(*stream, header))
	header.loadPalette = true;

	scenegraph::SceneGraph sceneGraph;
	wrapBool(readNodes(filename, *stream, sceneGraph, sceneGraph.root().id(), palette, header))

	Log::debug("qbcl: loaded %i colors", palette.colorCount());
	return palette.colorCount();
}

bool QBCLFormat::readMatrix(const core::String &filename, io::SeekableReadStream &stream,
							scenegraph::SceneGraph &sceneGraph, int parent, const core::String &name,
							palette::Palette &palette, Header &header, const NodeHeader &nodeHeader) {
	scenegraph::SceneGraphTransform transform;
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

	palette::PaletteLookup palLookup(palette);

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
					const color::RGBA color = flattenRGB(red, green, blue, 255 /* TODO: VOXELFORMAT: alpha support? */);
					if (header.loadPalette) {
						palette.tryAdd(color, false);
					} else {
						const uint8_t palIndex = palLookup.findClosestIndex(color);
						const voxel::Voxel &voxel = voxel::createVoxel(palette, palIndex);
						const uint32_t x = (index / size.z);
						const uint32_t z = index % size.z;
						voxel::RawVolume::Sampler sampler(volume);
						sampler.setPosition((int)x, y, (int)z);
						for (int j = 0; j < rleLength; ++j) {
							sampler.setVoxel(voxel);
							sampler.movePositiveY();
						}
						y += rleLength;
					}
				}
				// we've read another color value for the rle values
				++i;
			} else if (mask == 0) {
				++y;
			} else {
				// Uncompressed
				const color::RGBA color = flattenRGB(red, green, blue, 255 /* TODO: VOXELFORMAT: alpha support? */);
				if (header.loadPalette) {
					palette.tryAdd(color, false);
				} else {
					const uint32_t x = (index / size.z);
					const uint32_t z = index % size.z;
					const uint8_t palIndex = palLookup.findClosestIndex(color);
					const voxel::Voxel &voxel = voxel::createVoxel(palette, palIndex);
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

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume.release(), true);
	node.setVisible(nodeHeader.visible);
	node.setLocked(nodeHeader.locked);
	node.setPalette(palLookup.palette());
	if (name.empty()) {
		node.setName("Matrix");
	} else {
		node.setName(name);
	}
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	// the pivot is given in voxel coordinates
	// node.setPivot(pivot / glm::vec3(size)); // TODO: VOXELFORMAT:
	const int id = sceneGraph.emplace(core::move(node), parent);
	return id != -1;
}

bool QBCLFormat::readModel(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, int parent, const core::String &name,
						   palette::Palette &palette, Header &header, const NodeHeader &nodeHeader) {
	const size_t skip = 3 * 3 * sizeof(float);
	stream.skip((int64_t)skip); // TODO: VOXELFORMAT: rotation matrix?
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	if (name.empty()) {
		node.setName("Model");
	} else {
		node.setName(name);
	}
	node.setVisible(nodeHeader.visible);
	node.setLocked(nodeHeader.locked);
	int nodeId = parent == -1 ? sceneGraph.root().id() : sceneGraph.emplace(core::move(node), parent);
	Log::debug("Found %u children in model '%s'", childCount, name.c_str());
	for (uint32_t i = 0; i < childCount; ++i) {
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId, palette, header))
	}
	return true;
}

bool QBCLFormat::readCompound(const core::String &filename, io::SeekableReadStream &stream,
							  scenegraph::SceneGraph &sceneGraph, int parent, const core::String &name,
							  palette::Palette &palette, Header &header, const NodeHeader &nodeHeader) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	if (name.empty()) {
		node.setName("Compound");
	} else {
		node.setName(name);
	}
	node.setVisible(nodeHeader.visible);
	node.setLocked(nodeHeader.locked);
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	wrapBool(readMatrix(filename, stream, sceneGraph, nodeId, name, palette, header, nodeHeader))
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	Log::debug("Found %u children in compound '%s'", childCount, name.c_str());
	for (uint32_t i = 0; i < childCount; ++i) {
		wrapBool(readNodes(filename, stream, sceneGraph, nodeId, palette, header))
	}
	return true;
}

bool QBCLFormat::readNodes(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, int parent, palette::Palette &palette, Header &header) {
	uint32_t type;
	wrap(stream.readUInt32(type))
	uint32_t unknown;
	wrap(stream.readUInt32(unknown));
	Log::debug("unknown int: %u", unknown);

	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	NodeHeader nodeHeader;
	nodeHeader.visible = stream.readBool();
	nodeHeader.unknown = stream.readBool();
	nodeHeader.locked = stream.readBool();
	switch (type) {
	case qbcl::NODE_TYPE_MATRIX:
		core_assert(parent != -1);
		Log::debug("Found matrix");
		if (!readMatrix(filename, stream, sceneGraph, parent, name, palette, header, nodeHeader)) {
			Log::error("Failed to load matrix %s", name.c_str());
			return false;
		}
		Log::debug("Matrix of size %u loaded", unknown);
		break;
	case qbcl::NODE_TYPE_MODEL:
		Log::debug("Found model");
		if (!readModel(filename, stream, sceneGraph, parent, name, palette, header, nodeHeader)) {
			Log::error("Failed to load model %s", name.c_str());
			return false;
		}
		Log::debug("Model of size %u loaded", unknown);
		break;
	case qbcl::NODE_TYPE_COMPOUND:
		core_assert(parent != -1);
		Log::debug("Found compound");
		if (!readCompound(filename, stream, sceneGraph, parent, name, palette, header, nodeHeader)) {
			Log::error("Failed to load compound %s", name.c_str());
			return false;
		}
		Log::debug("Compound of size %u loaded", unknown);
		break;
	default:
		Log::warn("Unknown type found: '%s'", name.c_str());
		return false;
	}
	return true;
}

bool QBCLFormat::readHeader(io::SeekableReadStream &stream, Header &header) {
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
	wrap(stream.readUInt64(header.timestamp1))
	wrap(stream.readUInt64(header.timestamp2))
	return true;
}

bool QBCLFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	Header header;
	wrapBool(readHeader(*stream, header))

	palette::Palette palCopy = palette;
	wrapBool(readNodes(filename, *stream, sceneGraph, -1, palCopy, header))

	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty(scenegraph::PropTitle, header.title);
	rootNode.setProperty(scenegraph::PropDescription, header.desc);
	rootNode.setProperty(scenegraph::PropMetadata, header.metadata);
	rootNode.setProperty(scenegraph::PropAuthor, header.author);
	rootNode.setProperty(scenegraph::PropCompany, header.company);
	rootNode.setProperty(scenegraph::PropWebsite, header.website);
	rootNode.setProperty(scenegraph::PropCopyright, header.copyright);

	return true;
}

image::ImagePtr QBCLFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return image::ImagePtr();
	}
	uint32_t magic;
	wrapImg(stream->readUInt32(magic))
	if (magic != FourCC('Q', 'B', 'C', 'L')) {
		Log::error("Invalid magic found - no qbcl file");
		return image::ImagePtr();
	}
	uint32_t version;
	wrapImg(stream->readUInt32(version))
	uint32_t flags;
	wrapImg(stream->readUInt32(flags))
	uint32_t thumbWidth;
	wrapImg(stream->readUInt32(thumbWidth))
	uint32_t thumbHeight;
	wrapImg(stream->readUInt32(thumbHeight))
	if (thumbWidth <= 0 || thumbHeight <= 0) {
		Log::debug("No embedded screenshot found in %s", filename.c_str());
		return image::ImagePtr();
	}
	image::ImagePtr img = image::createEmptyImage(core::string::extractFilename(filename));
	if (!img->loadBGRA(*stream, thumbWidth, thumbHeight)) {
		Log::error("Failed to read the qbcl thumbnail buffer of width %u and height %u", thumbWidth, thumbHeight);
		return image::ImagePtr();
	}
	return img;
}

} // namespace voxelformat

#undef wrapImg
#undef wrap
#undef wrapBool
#undef wrapSaveNegative
#undef wrapSave
