/**
 * @file
 */

#include "scenegraph/SceneGraphAnimation.h"
#include "voxelformat/FormatThumbnail.h"
#include "QBCLFormat.h"
#include "core/Assert.h"
#include "color/Color.h"
#include "core/FourCC.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/StreamUtil.h"
#include "io/Base64.h"
#include "io/MemoryReadStream.h"
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
#include "math/Vec3u.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Conversions.h"

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
	const math::Vec3f translation = transform.localTranslation() - node.pivot() * math::Vec3f(region.getDimensionsInVoxels()) + math::Vec3f(region.getLowerCorner());
	const math::Vec3i &mins = region.getLowerCorner();
	const math::Vec3i &maxs = region.getUpperCorner();
	const math::Vec3i size = region.getDimensionsInVoxels();

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

	const math::Vec3f &normalizedPivot = node.pivot();
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
				if (voxel.isSameType(Empty)) {
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
	for (const core::UUID &childUUID : node.children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		if (child == nullptr) {
			continue;
		}
		wrapSave(saveNode(stream, sceneGraph, *child))
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
	const core::String modelData = node.property("qbcl_model_data");
	if (!modelData.empty()) {
		io::BufferedReadWriteStream decoded;
		if (!io::Base64::decode(decoded, modelData) || decoded.size() != 36 ||
			stream.write(decoded.getBuffer(), 36) != 36) {
			return false;
		}
	} else {
		// This block's semantics are unknown. Use Qubicle's default rather than
		// interpreting the bytes as a matrix (the first three integers are 1).
		for (int i = 0; i < 9; ++i) {
			wrapSave(stream.writeUInt32(i < 3 ? 1 : 0))
		}
	}
	wrapSave(stream.writeUInt32(children));

	for (const core::UUID &childUUID : node.children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		if (child == nullptr) {
			continue;
		}
		wrapSave(saveNode(stream, sceneGraph, *child))
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
	ctx.outputSize = math::Vec2i(128);
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

	palette::RGBABuffer colors;
	wrapBool(readNodes(*stream, nullptr, InvalidNodeId, &colors, ctx))
	return createPalette(colors, palette);
}

template<class Visitor>
bool QBCLFormat::readVoxels(io::SeekableReadStream &stream, const math::Vec3u &size, uint32_t compressedSize,
							Visitor &&visit) {
	const int64_t matrixEnd = stream.pos() + compressedSize;
	io::ZipReadStream zipStream(stream, (int)compressedSize);
	for (uint32_t x = 0; x < size.x; ++x) {
		for (uint32_t z = 0; z < size.z; ++z) {
			uint32_t y = 0;
			uint16_t rleEntries;
			wrap(zipStream.readUInt16(rleEntries))
			for (int i = 0; i < rleEntries; ++i) {
				color::RGBA color;
				wrapBool(io::readColor(zipStream, color))
				uint32_t count = 1;
				if (color.a == qbcl::RLE_FLAG) {
					count = color.r;
					if (++i >= rleEntries) {
						Log::error("Missing QBCL voxel run color");
						return false;
					}
					wrapBool(io::readColor(zipStream, color))
				}
				if (count == 0 || count > size.y - y) {
					Log::error("Invalid QBCL voxel run");
					return false;
				}
				if (color.a != 0) {
					visit(flattenRGB(color.r, color.g, color.b), x, y, z, count);
				}
				y += count;
			}
			if (y != size.y) {
				Log::error("QBCL column has %u voxels, expected %u", y, size.y);
				return false;
			}
		}
	}
	if (!zipStream.eos()) {
		uint8_t trailing;
		if (zipStream.read(&trailing, 1) != 0 || !zipStream.eos()) {
			Log::error("Trailing QBCL voxel columns");
			return false;
		}
	}
	return stream.seek(matrixEnd) != -1;
}

bool QBCLFormat::readMatrix(io::SeekableReadStream &stream, scenegraph::SceneGraph *sceneGraph, int parent,
							const core::String &name, const NodeHeader &nodeHeader, palette::RGBABuffer *colors) {
	math::Vec3u size;
	wrap(stream.readUInt32(size.x))
	wrap(stream.readUInt32(size.y))
	wrap(stream.readUInt32(size.z))

	math::Vec3i translation;
	wrap(stream.readInt32(translation.x))
	wrap(stream.readInt32(translation.y))
	wrap(stream.readInt32(translation.z))

	math::Vec3f pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	uint32_t compressedSize;
	wrap(stream.readUInt32(compressedSize))
	if (compressedSize == 0 || compressedSize > 0xFFFFFF || compressedSize > stream.remaining() ||
		math::any(math::greaterThan(size, math::Vec3u(MaxRegionSize))) ||
		math::any(math::lessThan(size, math::Vec3u(1)))) {
		Log::error("Invalid QBCL matrix size: %u:%u:%u with %u bytes", size.x, size.y, size.z, compressedSize);
		return false;
	}
	if (colors) {
		const auto collect = [colors](color::RGBA color, uint32_t, uint32_t, uint32_t, uint32_t) {
			colors->insert(color);
		};
		return readVoxels(stream, size, compressedSize, collect);
	}

	palette::Palette palette;
	if (core::getVar(cfg::VoxelCreatePalette)->boolVal()) {
		io::ScopedStreamPos streamPos(stream);
		palette::RGBABuffer matrixColors;
		const auto collect = [&matrixColors](color::RGBA color, uint32_t, uint32_t, uint32_t, uint32_t) {
			matrixColors.insert(color);
		};
		wrapBool(readVoxels(stream, size, compressedSize, collect))
		createPalette(matrixColors, palette);
	}
	if (palette.colorCount() == 0) {
		palette = voxel::getPalette();
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.createVolume(voxel::Region::fromSize(math::Vec3i(size)));
	palette::PaletteLookup lookup(palette, palette::PaletteLookup::Precision::Exact);
	const auto place = [&](color::RGBA color, uint32_t x, uint32_t y, uint32_t z, uint32_t count) {
		const voxel::Voxel voxel = voxel::createVoxel(palette, lookup.findClosestIndex(color));
		voxel::RawVolume::Sampler sampler(node.volume());
		sampler.setPosition((int)x, (int)y, (int)z);
		for (uint32_t i = 0; i < count; ++i) {
			sampler.setVoxel(voxel);
			sampler.movePositiveY();
		}
	};
	wrapBool(readVoxels(stream, size, compressedSize, place))
	node.setVisible(nodeHeader.visible);
	node.setLocked(nodeHeader.locked);
	node.setPalette(palette);
	node.setName(name.empty() ? "Matrix" : name);
	// Vengi stores translation at the pivot; QBCL stores the grid origin.
	node.setPivot(pivot / math::Vec3f(size));
	node.transform(0).setLocalTranslation(math::Vec3f(translation) + pivot);
	return sceneGraph->emplace(core::move(node), parent) != InvalidNodeId;
}

bool QBCLFormat::readNodes(io::SeekableReadStream &stream, scenegraph::SceneGraph *sceneGraph, int parent,
							palette::RGBABuffer *colors, const LoadContext &ctx) {
	uint32_t type;
	wrap(stream.readUInt32(type))
	uint32_t unknown;
	wrap(stream.readUInt32(unknown))
	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	NodeHeader nodeHeader;
	nodeHeader.visible = stream.readBool();
	nodeHeader.unknown = stream.readBool();
	nodeHeader.locked = stream.readBool();

	int nodeId = parent == InvalidNodeId ? 0 : parent;
	switch (type) {
	case qbcl::NODE_TYPE_MATRIX:
		return readMatrix(stream, sceneGraph, nodeId, name, nodeHeader, colors);
	case qbcl::NODE_TYPE_MODEL: {
		uint8_t modelBytes[36];
		if (stream.read(modelBytes, sizeof(modelBytes)) != sizeof(modelBytes)) {
			return false;
		}
		if (sceneGraph) {
			io::MemoryReadStream modelStream(modelBytes, sizeof(modelBytes));
			const core::String modelData = io::Base64::encode(modelStream);
			if (parent == InvalidNodeId) {
				scenegraph::SceneGraphNode &root = sceneGraph->node(nodeId);
				root.setName(name.empty() ? "Model" : name);
				root.setVisible(nodeHeader.visible);
				root.setLocked(nodeHeader.locked);
				root.setProperty("qbcl_model_data", modelData);
			} else {
				scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
				node.setName(name.empty() ? "Model" : name);
				node.setVisible(nodeHeader.visible);
				node.setLocked(nodeHeader.locked);
				node.setProperty("qbcl_model_data", modelData);
				nodeId = sceneGraph->emplace(core::move(node), parent);
			}
		}
		break;
	}
	case qbcl::NODE_TYPE_COMPOUND:
		if (sceneGraph) {
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
			node.setName(name.empty() ? "Compound" : name);
			node.setVisible(nodeHeader.visible);
			node.setLocked(nodeHeader.locked);
			nodeId = sceneGraph->emplace(core::move(node), nodeId);
			if (nodeId == InvalidNodeId) {
				return false;
			}
		}
		wrapBool(readMatrix(stream, sceneGraph, nodeId, name, nodeHeader, colors))
		break;
	default:
		Log::warn("Unknown QBCL node type %u: '%s'", type, name.c_str());
		return false;
	}

	if (nodeId == InvalidNodeId) {
		return false;
	}
	uint32_t childCount;
	wrap(stream.readUInt32(childCount))
	for (uint32_t i = 0; i < childCount; ++i) {
		ctx.report("node", (int)i, (int)childCount);
		wrapBool(readNodes(stream, sceneGraph, nodeId, colors, ctx))
	}
	ctx.report("node", (int)childCount, (int)childCount);
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

bool QBCLFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	Header header;
	wrapBool(readHeader(*stream, header))

	wrapBool(readNodes(*stream, &sceneGraph, InvalidNodeId, nullptr, ctx))

	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty(scenegraph::PropTitle, header.title);
	rootNode.setProperty(scenegraph::PropDescription, header.desc);
	rootNode.setProperty(scenegraph::PropMetadata, header.metadata);
	rootNode.setProperty(scenegraph::PropAuthor, header.author);
	rootNode.setProperty(scenegraph::PropCompany, header.company);
	rootNode.setProperty(scenegraph::PropWebsite, header.website);
	rootNode.setProperty(scenegraph::PropCopyright, header.copyright);
	sceneGraph.updateTransforms();
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
