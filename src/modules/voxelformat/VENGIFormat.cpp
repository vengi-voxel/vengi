/**
 * @file
 */

#include "VENGIFormat.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "glm/gtc/type_ptr.hpp"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"

#define wrapBool(action) \
	if ((action) != true) { \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrap(action) \
	if ((action) == -1) { \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__); \
		return false; \
	}

namespace voxelformat {

static scenegraph::SceneGraphNodeType toNodeType(const core::String &type) {
	for (int i = 0; i < lengthof(scenegraph::SceneGraphNodeTypeStr); ++i) {
		if (type == scenegraph::SceneGraphNodeTypeStr[i]) {
			return (scenegraph::SceneGraphNodeType)i;
		}
	}
	return scenegraph::SceneGraphNodeType::Max;
}

static scenegraph::InterpolationType toInterpolationType(const core::String &type) {
	for (int i = 0; i < lengthof(scenegraph::InterpolationTypeStr); ++i) {
		if (type == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Max;
}

bool VENGIFormat::saveNodeProperties(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	const core::StringMap<core::String> &properties = node.properties();
	if (properties.empty()) {
		return true;
	}
	wrapBool(stream.writeUInt32(FourCC('P', 'R', 'O', 'P')))
	wrapBool(stream.writeUInt32(properties.size()))
	for (const auto & e : properties) {
		wrapBool(stream.writePascalStringUInt16LE(e->key))
		wrapBool(stream.writePascalStringUInt16LE(e->value))
	}
	return true;
}

bool VENGIFormat::saveAnimation(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, const core::String &animation, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('A','N','I','M')))
	wrapBool(stream.writePascalStringUInt16LE(animation))
	for (const scenegraph::SceneGraphKeyFrame &keyframe : node.keyFrames()) {
		wrapBool(saveNodeKeyFrame(sceneGraph, keyframe, stream))
	}
	wrapBool(stream.writeUInt32(FourCC('E','N','D','A')))
	return true;
}

bool VENGIFormat::saveNodeData(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	if (node.type() != scenegraph::SceneGraphNodeType::Model) {
		return true;
	}
	wrapBool(stream.writeUInt32(FourCC('D','A','T','A')))
	const voxel::RawVolume *v = node.volume();
	const voxel::Region &region = v->region();
	wrapBool(stream.writeInt32(region.getLowerX()))
	wrapBool(stream.writeInt32(region.getLowerY()))
	wrapBool(stream.writeInt32(region.getLowerZ()))
	wrapBool(stream.writeInt32(region.getUpperX()))
	wrapBool(stream.writeInt32(region.getUpperY()))
	wrapBool(stream.writeInt32(region.getUpperZ()))
	auto visitor = [&stream] (int, int, int, const voxel::Voxel &voxel) {
		const bool air = isAir(voxel.getMaterial());
		stream.writeBool(air);
		if (!air) {
			stream.writeUInt8(voxel.getColor());
		}
	};
	voxelutil::visitVolume(*v, visitor, voxelutil::VisitAll(), voxelutil::VisitorOrder::XYZ);
	return true;
}

bool VENGIFormat::saveNodeKeyFrame(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphKeyFrame &keyframe, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('K','E','Y','F')))
	wrapBool(stream.writeUInt32(keyframe.frameIdx))
	wrapBool(stream.writeBool(keyframe.longRotation))
	wrapBool(stream.writePascalStringUInt16LE(scenegraph::InterpolationTypeStr[(int)keyframe.interpolation]))
	const scenegraph::SceneGraphTransform &transform = keyframe.transform();
	const float *localMatrix = glm::value_ptr(transform.localMatrix());
	for (int i = 0; i < 16; ++i) {
		wrapBool(stream.writeFloat(localMatrix[i]))
	}
	const glm::vec3 &pivot = transform.pivot();
	wrapBool(stream.writeFloat(pivot.x))
	wrapBool(stream.writeFloat(pivot.y))
	wrapBool(stream.writeFloat(pivot.z))
	return true;
}

bool VENGIFormat::saveNodePaletteColors(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('P','A','L','C')))
	const voxel::Palette &palette = node.palette();
	wrapBool(stream.writeUInt32(palette.colorCount()))
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt32(palette.color(i).rgba))
	}
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt32(palette.glowColor(i).rgba))
	}
	const voxel::PaletteIndicesArray &indices = palette.indices();
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt8(indices[i]))
	}
	wrapBool(stream.writeUInt32(0)) // TODO: slot for amount of material properties
	return true;
}

bool VENGIFormat::saveNodePaletteIdentifier(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('P','A','L','I')))
	wrapBool(stream.writePascalStringUInt16LE(node.palette().name()))
	return true;
}

bool VENGIFormat::saveNode(const scenegraph::SceneGraph &sceneGraph, io::WriteStream& stream, const scenegraph::SceneGraphNode& node) {
	wrapBool(stream.writeUInt32(FourCC('N','O','D','E')))
	wrapBool(stream.writePascalStringUInt16LE(node.name()))
	wrapBool(stream.writePascalStringUInt16LE(scenegraph::SceneGraphNodeTypeStr[(int)node.type()]))
	wrapBool(stream.writeInt32(node.id()))
	wrapBool(stream.writeInt32(node.reference()))
	wrapBool(stream.writeBool(node.visible()))
	wrapBool(stream.writeBool(node.locked()))
	wrapBool(stream.writeUInt32(node.color().rgba))
	wrapBool(saveNodeProperties(sceneGraph, node, stream))
	if (node.palette().isBuiltIn()) {
		wrapBool(saveNodePaletteIdentifier(sceneGraph, node, stream))
	} else {
		wrapBool(saveNodePaletteColors(sceneGraph, node, stream))
	}
	wrapBool(saveNodeData(sceneGraph, node, stream))
	for (const core::String &animation : sceneGraph.animations()) {
		wrapBool(saveAnimation(sceneGraph, node, animation, stream))
	}
	for (int childId : node.children()) {
		wrapBool(saveNode(sceneGraph, stream, sceneGraph.node(childId)))
	}
	wrapBool(stream.writeUInt32(FourCC('E','N','D','N')))
	return true;
}

bool VENGIFormat::loadNodeProperties(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	uint32_t propertyCount;
	wrap(stream.readUInt32(propertyCount))
	Log::debug("Load %u properties", propertyCount);
	for (uint32_t i = 0; i < propertyCount; ++i) {
		core::String key, value;
		wrapBool(stream.readPascalStringUInt16LE(key))
		wrapBool(stream.readPascalStringUInt16LE(value))
		node.setProperty(key, value);
	}
	return true;
}

bool VENGIFormat::loadNodeData(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	glm::ivec3 mins, maxs;
	wrap(stream.readInt32(mins.x))
	wrap(stream.readInt32(mins.y))
	wrap(stream.readInt32(mins.z))
	wrap(stream.readInt32(maxs.x))
	wrap(stream.readInt32(maxs.y))
	wrap(stream.readInt32(maxs.z))
	Log::debug("Load region of %i:%i:%i %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	const voxel::Region region(mins, maxs);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setVolume(v, true);
	const voxel::Palette &palette = node.palette();

	auto visitor = [&stream, v, &palette] (int x, int y, int z, const voxel::Voxel &voxel) {
		const bool air = stream.readBool();
		if (air) {
			return;
		}
		uint8_t color;
		stream.readUInt8(color);
		const voxel::VoxelType type = palette.color(color).a != 255 ? voxel::VoxelType::Transparent : voxel::VoxelType::Generic;
		v->setVoxel(x, y, z, voxel::createVoxel(type, color));
	};
	voxelutil::visitVolume(*v, visitor, voxelutil::VisitAll(), voxelutil::VisitorOrder::XYZ);
	return true;
}

bool VENGIFormat::loadNodePaletteColors(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	voxel::Palette palette;
	uint32_t colorCount;
	wrap(stream.readUInt32(colorCount))
	Log::debug("Load node palette with %u color", colorCount);
	palette.setSize((int)colorCount);
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt32(palette.color(i).rgba))
	}
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt32(palette.glowColor(i).rgba))
	}
	voxel::PaletteIndicesArray &indices = palette.indices();
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt8(indices[i]))
	}
	uint32_t palettePropertyCnt;
	wrap(stream.readUInt32(palettePropertyCnt)) // TODO: slot for further extensions
	node.setPalette(palette);
	return true;
}

bool VENGIFormat::loadNodePaletteIdentifier(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	core::String name;
	wrapBool(stream.readPascalStringUInt16LE(name))
	voxel::Palette palette;
	Log::debug("Load node palette %s", name.c_str());
	palette.load(name.c_str());
	if (palette.colorCount() == 0) {
		Log::error("Failed to load built-in palette %s", name.c_str());
		return false;
	}
	node.setPalette(palette);
	return true;
}

bool VENGIFormat::loadAnimation(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	core::String animation;
	wrapBool(stream.readPascalStringUInt16LE(animation))
	Log::debug("Load node animation %s", animation.c_str());
	sceneGraph.addAnimation(animation);
	while (!stream.eos()) {
		uint32_t chunkMagic;
		wrap(stream.readUInt32(chunkMagic))
		if (chunkMagic == FourCC('K','E','Y','F')) {
			if (!loadNodeKeyFrame(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('E','N','D','A')) {
			return true;
		}
	}
	Log::error("ENDA magic is missing");
	return false;
}

bool VENGIFormat::loadNodeKeyFrame(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream) {
	scenegraph::FrameIndex frameIdx = 0;
	wrap(stream.readInt32(frameIdx))
	scenegraph::KeyFrameIndex keyFrameIdx = node.addKeyFrame(frameIdx);
	if (keyFrameIdx == InvalidKeyFrame) {
		keyFrameIdx = node.keyFrameForFrame(frameIdx);
	}
	scenegraph::SceneGraphKeyFrame &keyframe = node.keyFrame(keyFrameIdx);
	keyframe.longRotation = stream.readBool();
	core::String interpolationType;
	wrapBool(stream.readPascalStringUInt16LE(interpolationType))
	keyframe.interpolation = toInterpolationType(interpolationType);
	Log::debug("Load animation keyframe %u: %s", frameIdx, interpolationType.c_str());
	scenegraph::SceneGraphTransform &transform = keyframe.transform();
	glm::mat4 localMatrix;
	float *localMatrixPtr = glm::value_ptr(localMatrix);
	for (int i = 0; i < 16; ++i) {
		wrap(stream.readFloat(localMatrixPtr[i]))
	}
	transform.setLocalMatrix(localMatrix);
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))
	transform.setPivot(pivot);
	return true;
}

bool VENGIFormat::loadNode(scenegraph::SceneGraph &sceneGraph, int parent, uint32_t version, io::ReadStream& stream, NodeMapping &nodeMapping) {
	core::String name;
	wrapBool(stream.readPascalStringUInt16LE(name))
	core::String type;
	wrapBool(stream.readPascalStringUInt16LE(type))
	scenegraph::SceneGraphNodeType nodeType = toNodeType(type);
	if (nodeType == scenegraph::SceneGraphNodeType::Max) {
		Log::error("Could not load node type %s", type.c_str());
		return false;
	}
	Log::debug("Load node with name '%s' of type %s", name.c_str(), type.c_str());
	int nodeId = nodeType == scenegraph::SceneGraphNodeType::Root ? sceneGraph.root().id() : InvalidNodeId;
	if (nodeId == InvalidNodeId) {
		scenegraph::SceneGraphNode node(nodeType);
		node.setName(name);
		if (nodeType == scenegraph::SceneGraphNodeType::Model) {
			// dummy volume - will be replaced later
			node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		}
		nodeId = sceneGraph.emplace(core::move(node), parent);
		if (nodeId == -1) {
			Log::error("Failed to add new node");
			return false;
		}
	}
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);

	if (version >= 2) {
		int fileNodeId;
		wrap(stream.readInt32(fileNodeId))
		int referenceNodeId;
		wrap(stream.readInt32(referenceNodeId))
		// will get fixed up later once we know all node ids
		node.setReference(referenceNodeId);
		nodeMapping.put(fileNodeId, nodeId);
	}
	node.setVisible(stream.readBool());
	node.setLocked(stream.readBool());
	core::RGBA color;
	wrap(stream.readUInt32(color.rgba))
	node.setColor(color);

	while (!stream.eos()) {
		uint32_t chunkMagic;
		wrap(stream.readUInt32(chunkMagic))
		if (chunkMagic == FourCC('P','R','O','P')) {
			if (!loadNodeProperties(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('D','A','T','A')) {
			if (!loadNodeData(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('P','A','L','C')) {
			if (!loadNodePaletteColors(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('P','A','L','I')) {
			if (!loadNodePaletteIdentifier(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('A','N','I','M')) {
			if (!loadAnimation(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('N','O','D','E')) {
			if (!loadNode(sceneGraph, node.id(), version, stream, nodeMapping)) {
				return false;
			}
		} else if (chunkMagic == FourCC('E','N','D','N')) {
			return true;
		}
	}
	Log::error("ENDN magic is missing");
	return false;
}

bool VENGIFormat::saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) {
	wrapBool(stream.writeUInt32(FourCC('V','E','N','G')))
	io::ZipWriteStream zipStream(stream);
	wrapBool(zipStream.writeUInt32(2))
	if (!saveNode(sceneGraph, zipStream, sceneGraph.root())) {
		return false;
	}
	return true;
}

bool VENGIFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, const LoadContext &ctx) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('V','E','N','G')) {
		Log::error("Invalid magic");
		return false;
	}
	io::ZipReadStream zipStream(stream);
	uint32_t version;
	wrap(zipStream.readUInt32(version))
	if (version > 2) {
		Log::error("Unsupported version %u", version);
		return false;
	}
	uint32_t chunkMagic;
	wrap(zipStream.readUInt32(chunkMagic))
	NodeMapping nodeMapping;
	if (chunkMagic == FourCC('N','O','D','E')) {
		if (!loadNode(sceneGraph, sceneGraph.root().id(), version, zipStream, nodeMapping)) {
			return false;
		}
		for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::ModelReference); iter != sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			int nodeId;
			if (!nodeMapping.get(node.reference(), nodeId)) {
				Log::error("Failed to perform node id mapping for references");
				return false;
			}
			Log::debug("Update node reference for node %i to: %i", node.id(), nodeId);
			node.setReference(nodeId);
		}
		sceneGraph.updateTransforms();
		return true;
	}
	Log::error("Unknown chunk magic");
	return false;
}

#undef wrap
#undef wrapBool

}
