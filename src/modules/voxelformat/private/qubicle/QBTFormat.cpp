/**
 * @file
 */

#include "QBTFormat.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>

namespace voxelformat {

namespace qbt {
const int NODE_TYPE_MATRIX = 0;
const int NODE_TYPE_MODEL = 1;
const int NODE_TYPE_COMPOUND = 2;

class ScopedQBTHeader {
private:
	io::SeekableWriteStream &_stream;
	uint32_t _sizePos = 0;
	bool _success = true;

public:
	ScopedQBTHeader(io::SeekableWriteStream &stream, uint32_t nodeType) : _stream(stream) {
		Log::debug("Write node type %u", nodeType);
		if (!_stream.writeUInt32(nodeType)) {
			Log::error("Failed to write the node type %u", nodeType);
			_success = false;
		}
		_sizePos = _stream.pos();
		if (!_stream.writeUInt32(0)) {
			Log::error("Failed to write the node size");
			_success = false;
		}
	}

	ScopedQBTHeader(io::SeekableWriteStream &stream, scenegraph::SceneGraphNodeType type) : _stream(stream) {
		uint32_t nodeType = 0;
		switch (type) {
		case scenegraph::SceneGraphNodeType::Group:
		case scenegraph::SceneGraphNodeType::Root:
			nodeType = qbt::NODE_TYPE_MODEL;
			Log::debug("Write model node");
			break;
		case scenegraph::SceneGraphNodeType::Model:
		case scenegraph::SceneGraphNodeType::ModelReference:
			nodeType = qbt::NODE_TYPE_MATRIX;
			Log::debug("Write matrix node");
			break;
		default:
			Log::error("Failed to determine the node type for %u", (uint32_t)type);
			_success = false;
			break;
		}

		if (!_stream.writeUInt32(nodeType)) {
			Log::error("Failed to write the node type %u", nodeType);
			_success = false;
		}
		_sizePos = _stream.pos();
		if (!_stream.writeUInt32(0)) {
			Log::error("Failed to write the node size");
			_success = false;
		}
	}

	~ScopedQBTHeader() {
		const uint32_t dataEnd = _stream.pos();
		const uint32_t delta = dataEnd - _sizePos - sizeof(uint32_t);
		if (_stream.seek(_sizePos) == -1) {
			Log::error("Failed to seek to size pos %u", _sizePos);
			_success = false;
			return;
		}
		Log::debug("Write node size %u", delta);
		if (!_stream.writeUInt32(delta)) {
			_success = false;
			Log::error("Failed to write node size %u", delta);
			return;
		}
		if (_stream.seek(dataEnd) == -1) {
			Log::error("Failed to seek to eos %u", dataEnd);
			_success = false;
			return;
		}
		if (!_success) {
			Log::error("Failed to finish the node header");
		}
	}

	inline bool success() const {
		return _success;
	}
};

} // namespace qbt

#define wrapSave(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save qbt file: " CORE_STRINGIFY(write) " failed");                                       \
		return false;                                                                                                  \
	}

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool QBTFormat::saveMatrix(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						   const scenegraph::SceneGraphNode &node, bool colorMap) const {
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const glm::ivec3 size = region.getDimensionsInVoxels();

	const palette::Palette &palette = node.palette();

	const int zlibBufSize = size.x * size.y * size.z * (int)sizeof(uint32_t);
	io::BufferedReadWriteStream bufferStream(zlibBufSize);
	io::ZipWriteStream zipStream(bufferStream);

	const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int z = mins.z; z <= maxs.z; ++z) {
			for (int y = mins.y; y <= maxs.y; ++y) {
				const voxel::Voxel &voxel = v->voxel(x, y, z);
				if (isAir(voxel.getMaterial())) {
					zipStream.writeUInt8(0);
					zipStream.writeUInt8(0);
					zipStream.writeUInt8(0);
					zipStream.writeUInt8(0); // mask 0 == air
					continue;
				}
				if (colorMap) {
					zipStream.writeUInt8(voxel.getColor());
					zipStream.writeUInt8(0);
					zipStream.writeUInt8(0);
				} else {
					const core::RGBA voxelColor = palette.color(voxel.getColor());
					// const uint8_t alpha = voxelColor.a * 255.0f;
					zipStream.writeUInt8(voxelColor.r);
					zipStream.writeUInt8(voxelColor.g);
					zipStream.writeUInt8(voxelColor.b);
				}
				// mask != 0 means solid, 1 is core (surrounded by others and not visible)
				// TODO: VOXELFORMAT: const voxel::FaceBits faceBits = voxel::visibleFaces(v, x, y, z);
				zipStream.writeUInt8(0xff);
			}
		}
	}

	zipStream.flush();

	wrapSave(stream.writePascalStringUInt32LE(node.name()));
	Log::debug("Save matrix with name %s", node.name().c_str());

	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::ivec3 offset = glm::round(transform.localTranslation());
	wrapSave(stream.writeInt32(offset.x));
	wrapSave(stream.writeInt32(offset.y));
	wrapSave(stream.writeInt32(offset.z));

	const glm::uvec3 localScale{1};
	wrapSave(stream.writeUInt32(localScale.x));
	wrapSave(stream.writeUInt32(localScale.y));
	wrapSave(stream.writeUInt32(localScale.z));

	const glm::vec3 &pivot = node.pivot();
	wrapSave(stream.writeFloat(pivot.x));
	wrapSave(stream.writeFloat(pivot.y));
	wrapSave(stream.writeFloat(pivot.z));

	wrapSave(stream.writeUInt32(size.x));
	wrapSave(stream.writeUInt32(size.y));
	wrapSave(stream.writeUInt32(size.z));

	Log::debug("save %i compressed bytes", (int)bufferStream.size());
	wrapSave(stream.writeUInt32(bufferStream.size()));
	if (stream.write(bufferStream.getBuffer(), bufferStream.size()) == -1) {
		Log::error("Could not save qbt file: failed to write the compressed buffer");
		return false;
	}

	return true;
}

bool QBTFormat::saveColorMap(io::SeekableWriteStream &stream, const palette::Palette &palette) const {
	wrapSave(stream.writeString("COLORMAP", false));
	wrapSave(stream.writeUInt32(palette.colorCount()));
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapSave(stream.writeUInt8(palette.color(i).r));
		wrapSave(stream.writeUInt8(palette.color(i).g));
		wrapSave(stream.writeUInt8(palette.color(i).b));
		wrapSave(stream.writeUInt8(palette.color(i).a));
	}

	return true;
}

bool QBTFormat::saveCompound(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
							 const scenegraph::SceneGraphNode &node, bool colorMap) const {
	wrapSave(saveMatrix(stream, sceneGraph, node, colorMap))
	wrapSave(stream.writeUInt32((int)node.children().size()));
	for (int nodeId : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
		wrapSave(saveNode(stream, sceneGraph, cnode, colorMap))
	}
	return true;
}

bool QBTFormat::saveNode(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						 const scenegraph::SceneGraphNode &node, bool colorMap) const {
	const scenegraph::SceneGraphNodeType type = node.type();
	if (node.isAnyModelNode()) {
		if (node.children().empty()) {
			qbt::ScopedQBTHeader header(stream, type);
			wrapSave(saveMatrix(stream, sceneGraph, node, colorMap) && header.success())
		} else {
			qbt::ScopedQBTHeader scoped(stream, qbt::NODE_TYPE_COMPOUND);
			wrapSave(saveCompound(stream, sceneGraph, node, colorMap) && scoped.success())
		}
	} else if (type == scenegraph::SceneGraphNodeType::Group || type == scenegraph::SceneGraphNodeType::Root) {
		wrapSave(saveModel(stream, sceneGraph, node, colorMap))
	}
	return true;
}

bool QBTFormat::saveModel(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						  const scenegraph::SceneGraphNode &node, bool colorMap) const {
	if (node.children().size() == 1) {
		for (int nodeId : node.children()) {
			const scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
			wrapSave(saveNode(stream, sceneGraph, cnode, colorMap))
		}
		return true;
	}
	qbt::ScopedQBTHeader scoped(stream, node.type());
	const int children = (int)node.children().size();
	wrapSave(stream.writeUInt32(children));
	for (int nodeId : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
		wrapSave(saveNode(stream, sceneGraph, cnode, colorMap))
	}
	return scoped.success();
}

bool QBTFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	const scenegraph::SceneGraphNodeChildren &children = root.children();
	const int childCount = (int)children.size();
	if (childCount <= 0) {
		Log::error("Empty scene graph - can't save qbt");
		return false;
	}

	wrapSave(stream->writeUInt32(FourCC('Q', 'B', ' ', '2')))
	wrapSave(stream->writeUInt8(1));	   // version
	wrapSave(stream->writeUInt8(0));	   // version
	wrapSave(stream->writeFloat(1.0f)); // globalscale
	wrapSave(stream->writeFloat(1.0f)); // globalscale
	wrapSave(stream->writeFloat(1.0f)); // globalscale
	const bool colorMap = core::Var::getSafe(cfg::VoxformatQBTPaletteMode)->boolVal();
	if (colorMap) {
		const palette::Palette &palette = sceneGraph.firstPalette();
		if (!saveColorMap(*stream, palette)) {
			return false;
		}
	} else {
		palette::Palette palette;
		if (!saveColorMap(*stream, palette)) {
			return false;
		}
	}
	if (!stream->writeString("DATATREE", false)) {
		return false;
	}
	return saveNode(*stream, sceneGraph, sceneGraph.root(), colorMap);
}

bool QBTFormat::skipNode(io::SeekableReadStream &stream) {
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
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this
 * node) NameLength  4 bytes Name NameLength bytes, char Position X, Y, Z 3 * 4 bytes, int, position relative to parent
 * node LocalScale X, Y, Z 3 * 4 bytes, uint Pivot X, Y, Z 3 * 4 bytes, float Size X, Y, Z 3 * 4 bytes, uint
 * CompoundVoxelDataSize 4 bytes, uint
 * CompoundVoxelData VoxelDataSize bytes, zlib compressed voxel data
 * ChildCount 4 bytes, uint, number of child nodes
 * Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadCompound(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
							 palette::Palette &palette, Header &state) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	node.setName("Compound");
	int nodeId = sceneGraph.emplace(core::move(node), parent);

	if (!loadMatrix(stream, sceneGraph, nodeId, palette, state)) {
		return false;
	}
	const bool mergeCompounds = core::Var::getSafe(cfg::VoxformatQBTMergeCompounds)->boolVal();
	uint32_t childCount;
	wrap(stream.readUInt32(childCount));
	Log::debug("Load %u children", childCount);
	for (uint32_t i = 0; i < childCount; ++i) {
		if (mergeCompounds) {
			// if you don't need the datatree you can skip child nodes
			if (!skipNode(stream)) {
				return false;
			}
		} else {
			if (!loadNode(stream, sceneGraph, nodeId, palette, state)) {
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
 * Voxel data is stored in a 3D grid. The data is compressed using zlib and stored in X, Y, Z with Y running fastest and
 * X running slowest. Each voxel uses 4 bytes: RGBM. RGB stores true color information and M the visibility Mask.
 *
 * If a color map is included then the R byte references to a color of the color map. In this case the G and B bytes may
 * contain additional secondary data references.
 *
 * The M byte is used to store visibility of the 6 faces of a voxel and whether as voxel is solid or air. If M is bigger
 * than 0 then the voxel is solid. Even when a voxel is solid is may not be needed to be rendered because it is a core
 * voxel that is surrounded by 6 other voxels and thus invisible. If M = 1 then the voxel is a core voxel.
 */
bool QBTFormat::loadMatrix(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
						   palette::Palette &palette, Header &state) {
	core::String name;
	wrapBool(stream.readPascalStringUInt32LE(name))
	Log::debug("Matrix name: %s", name.c_str());
	glm::ivec3 translation;
	scenegraph::SceneGraphTransform transform;
	wrap(stream.readInt32(translation.x))
	wrap(stream.readInt32(translation.y))
	wrap(stream.readInt32(translation.z))
	transform.setWorldTranslation(translation);

	glm::uvec3 localScale;
	wrap(stream.readUInt32(localScale.x))
	wrap(stream.readUInt32(localScale.y))
	wrap(stream.readUInt32(localScale.z))

	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	glm::uvec3 size;
	wrap(stream.readUInt32(size.x))
	wrap(stream.readUInt32(size.y))
	wrap(stream.readUInt32(size.z))

	uint32_t voxelDataSize;
	wrap(stream.readUInt32(voxelDataSize))
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
		Log::warn("Size of matrix results in empty space - voxelDataSize: %u", voxelDataSize);
		return false;
	}
	io::ZipReadStream zipStream(stream, voxelDataSize);

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> volume(new voxel::RawVolume(region));
	// TODO: PERF: use volume sampler
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
				if (state.colorFormat == ColorFormat::Palette) {
					const voxel::Voxel &voxel = voxel::createVoxel(palette, red);
					volume->setVoxel(x, y, z, voxel);
				} else {
					const core::RGBA color = flattenRGB(red, green, blue);
					uint8_t index = 1;
					palette.tryAdd(color, false, &index);
					const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
					volume->setVoxel(x, y, z, voxel);
				}
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume.release(), true);
	node.setName(name);
	node.setPivot(pivot);
	node.setPalette(palette);
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	const int id = sceneGraph.emplace(core::move(node), parent);
	return id != InvalidNodeId;
}

/**
 * Model Node
 * TypeID 4 bytes, uint = 1
 * DataSize 4 bytes, uint, number of bytes used for this node and all child nodes (excluding TypeID and DataSize of this
 * node) ChildCount 4 bytes, uint, number of child nodes Children ChildCount nodes currently of type Matrix or Compound
 */
bool QBTFormat::loadModel(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
						  palette::Palette &palette, Header &state) {
	uint32_t childCount;
	wrap(stream.readUInt32(childCount));
	if (childCount > 2048u) {
		Log::error("Max child count exceeded: %i", (int)childCount);
		return false;
	}
	Log::debug("Found %u children", childCount);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group);
	node.setName("Model");
	int nodeId = sceneGraph.emplace(core::move(node), parent);
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, sceneGraph, nodeId, palette, state)) {
			return false;
		}
	}
	return true;
}

/**
 * Data Tree
 * SectionCaption 8 bytes = "DATATREE"
 * RootNode, can currently either be Model, Compound or Matrix
 */
bool QBTFormat::loadNode(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, int parent,
						 palette::Palette &palette, Header &state) {
	uint32_t nodeTypeID;
	wrap(stream.readUInt32(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readUInt32(dataSize));
	Log::debug("Data size: %u", dataSize);

	switch (nodeTypeID) {
	case qbt::NODE_TYPE_MATRIX: {
		Log::debug("Found matrix");
		if (!loadMatrix(stream, sceneGraph, parent, palette, state)) {
			Log::error("Failed to load matrix");
			return false;
		}
		Log::debug("Matrix of size %u loaded", dataSize);
		break;
	}
	case qbt::NODE_TYPE_MODEL:
		Log::debug("Found model");
		if (!loadModel(stream, sceneGraph, parent, palette, state)) {
			Log::error("Failed to load model");
			return false;
		}
		Log::debug("Model of size %u loaded", dataSize);
		break;
	case qbt::NODE_TYPE_COMPOUND:
		Log::debug("Found compound");
		if (!loadCompound(stream, sceneGraph, parent, palette, state)) {
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

/**
 * Color Map
 * SectionCaption 8 bytes = "COLORMAP"
 * ColorCount 4 bytes, uint, if this value is 0 then no color map is used
 * Colors ColorCount * 4 bytes, rgba
 */
bool QBTFormat::loadColorMap(io::SeekableReadStream &stream, palette::Palette &palette) {
	uint32_t colorCount;
	wrap(stream.readUInt32(colorCount));
	Log::debug("Load color map with %u colors", colorCount);
	if (colorCount > palette::PaletteMaxColors) {
		Log::warn("Can't load all palette colors (%u)", colorCount);
	}
	palette.setSize(core_min((int)colorCount, palette::PaletteMaxColors));
	for (uint32_t i = 0; i < colorCount; ++i) {
		uint8_t colorByteR;
		uint8_t colorByteG;
		uint8_t colorByteB;
		uint8_t colorByteVisMask;
		wrap(stream.readUInt8(colorByteR));
		wrap(stream.readUInt8(colorByteG));
		wrap(stream.readUInt8(colorByteB));
		wrap(stream.readUInt8(colorByteVisMask));
		palette.setColor(i, core::RGBA(colorByteR, colorByteG, colorByteB));
	}
	return true;
}

/**
 * Header
 * Magic  4 bytes must be 0x32204251 = "QB 2"
 * VersionMajor 1 byte, currently = 1
 * VersionMinor 1 byte, currently = 0
 * GlobalScale X, Y, Z 3 * 4 bytes, float, normally 1, 1, 1, can be used in case voxels are not cubes (e.g. Lego Bricks)
 */
bool QBTFormat::loadHeader(io::SeekableReadStream &stream, Header &state) {
	uint32_t header;
	wrap(stream.readUInt32(header))
	constexpr uint32_t headerMagic = FourCC('Q', 'B', ' ', '2');
	if (header != headerMagic) {
		Log::error("Could not load qbt file: Invalid magic found (%u vs %u)", header, headerMagic);
		return false;
	}

	wrap(stream.readUInt8(state.versionMajor))
	wrap(stream.readUInt8(state.versionMinor))
	wrap(stream.readFloat(state.globalScale.x));
	wrap(stream.readFloat(state.globalScale.y));
	wrap(stream.readFloat(state.globalScale.z));
	Log::debug("QBT with version %u.%u", state.versionMajor, state.versionMinor);
	return true;
}

size_t QBTFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
	}
	Header state;
	if (!loadHeader(*stream, state)) {
		Log::error("Could not load qbt file: Could not read header");
		return 0u;
	}

	const int64_t pos = stream->pos();

	while (stream->remaining() > 0) {
		char buf[8];
		if (!stream->readString(sizeof(buf), buf)) {
			Log::error("Could not load qbt file: Could not read chunk id");
			return 0u;
		}
		if (0 == memcmp(buf, "COLORMAP", 7)) {
			if (!loadColorMap(*stream, palette)) {
				Log::error("Failed to load color map");
				return 0;
			}
			int colorCount = palette.colorCount();
			Log::debug("Load qbt palette with %i entries", colorCount);
			if (colorCount > 0) {
				return colorCount;
			}
		} else if (0 == memcmp(buf, "DATATREE", 8)) {
			wrapBool(skipNode(*stream))
		} else {
			Log::error("Unknown section found: %c%c%c%c%c%c%c%c", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
					   buf[6], buf[7]);
			break;
		}
	}
	Log::debug("no palette found");

	// no COLORMAP data was found
	stream->seek(pos);

	while (stream->remaining() > 0) {
		char buf[8];
		if (!stream->readString(sizeof(buf), buf)) {
			Log::error("Could not load qbt file: Could not read chunk id");
			return 0u;
		}
		if (0 == memcmp(buf, "DATATREE", 8)) {
			scenegraph::SceneGraph sceneGraph;
			if (!loadNode(*stream, sceneGraph, sceneGraph.root().id(), palette, state)) {
				Log::error("Failed to load node");
				return 0u;
			}
		} else {
			Log::error("Unknown section found: %c%c%c%c%c%c%c%c", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
					   buf[6], buf[7]);
			break;
		}
	}

	Log::debug("Failed to load qbt palette");
	return 0;
}

bool QBTFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	Header state;
	wrapBool(loadHeader(*stream, state))

	while (stream->remaining() > 0) {
		char buf[8];
		wrapBool(stream->readString(sizeof(buf), buf));
		if (0 == memcmp(buf, "COLORMAP", 7)) {
			if (!loadColorMap(*stream, palette)) {
				Log::error("Failed to load color map");
				return false;
			}
			if (palette.colorCount() == 0) {
				Log::debug("No color map found");
			} else {
				Log::debug("Color map loaded");
				state.colorFormat = ColorFormat::Palette;
			}
		} else if (0 == memcmp(buf, "DATATREE", 8)) {
			Log::debug("load data tree");
			if (!loadNode(*stream, sceneGraph, sceneGraph.root().id(), palette, state)) {
				Log::error("Failed to load node");
				return false;
			}
		} else {
			Log::error("Unknown section found: %c%c%c%c%c%c%c%c", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
					   buf[6], buf[7]);
			return false;
		}
	}
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		node.setPalette(palette);
	}
	return true;
}

#undef wrapSave
#undef wrap
#undef wrapBool

} // namespace voxelformat
