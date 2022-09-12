/**
 * @file
 */

#include "VXRFormat.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "app/App.h"
#include "VXMFormat.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelformat/VXAFormat.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vxr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vxr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXRFormat::saveRecursiveNode(const SceneGraph& sceneGraph, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream) {
	core::String name = node.name();
	if (name.empty()) {
		name = core::string::format("%i", node.id());
	}
	wrapBool(stream.writeString(node.name(), true))
	if (node.type() != SceneGraphNodeType::Model) {
		wrapBool(stream.writeString("", true))
	} else {
		const core::String baseName = core::string::stripExtension(core::string::extractFilename(filename));
		const core::String finalName = baseName + name + ".vxm";
		wrapBool(stream.writeString(finalName, true))
		const core::String fullPath = core::string::stripExtension(filename) + name + ".vxm";
		VXMFormat f;
		io::FilePtr outputFile = io::filesystem()->open(fullPath, io::FileMode::SysWrite);
		if (!outputFile) {
			Log::error("Failed to open %s for writing", fullPath.c_str());
			return false;
		}
		io::FileStream wstream(outputFile);
		SceneGraph newSceneGraph;
		SceneGraphNode newNode;
		copyNode(node, newNode, false);
		newSceneGraph.emplace(core::move(newNode));
		wrapBool(f.saveGroups(newSceneGraph, fullPath, wstream))
		Log::debug("Saved the model to %s", fullPath.c_str());
	}

	wrapBool(saveNodeProperties(&node, stream))

	const int32_t childCount = (int32_t)node.children().size();
	wrapBool(stream.writeInt32(childCount));
	for (int child : node.children()) {
		const voxelformat::SceneGraphNode &cnode = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, cnode, filename, stream))
	}
	return true;
}

bool VXRFormat::saveNodeProperties(const SceneGraphNode* node, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeBool(boolProperty(node, "collidable", true)))
	wrapBool(stream.writeBool(boolProperty(node, "decorative", false)))
	if (node == nullptr) {
		wrapBool(stream.writeUInt32(0))
	} else {
		wrapBool(stream.writeUInt32(node->color().rgba))
	}
	wrapBool(stream.writeBool(boolProperty(node, "favorite", false)))
	wrapBool(stream.writeBool(boolProperty(node, "folded", false)))
	wrapBool(stream.writeBool(boolProperty(node, "mirror x axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "mirror y axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "mirror z axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "preview mirror x axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "preview mirror y axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "preview mirror z axis", false)))
	wrapBool(stream.writeBool(boolProperty(node, "ikAnchor", false)))
	wrapBool(stream.writeString(stringProperty(node, "ikEffectorId"), true))
	wrapBool(stream.writeBool(boolProperty(node, "ikConstraintsVisible", true)))
	wrapBool(stream.writeFloat(floatProperty(node, "ikRollMin", 0.0f)))
	wrapBool(stream.writeFloat(floatProperty(node, "ikRollMax", glm::two_pi<float>())))
	int ikConstraintAmount = 0;
	wrapBool(stream.writeUInt32(ikConstraintAmount)) // (max 10)
	for (int i = 0; i < ikConstraintAmount; ++i) {
		wrapBool(stream.writeFloat(0.0f)) // ikConstraintX
		wrapBool(stream.writeFloat(0.0f)) // ikConstraintZ
		wrapBool(stream.writeFloat(0.0f)) // ikConstraintRadius
	}
	return true;
}

bool VXRFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	const voxelformat::SceneGraphNode &root = sceneGraph.root();
	const voxelformat::SceneGraphNodeChildren &children = root.children();
	const int childCount = (int)children.size();
	if (childCount <= 0) {
		Log::error("Empty scene graph - can't save vxr");
		return false;
	}
	wrapBool(stream.writeUInt32(FourCC('V','X','R','9')))
	SceneGraphAnimationIds animationIds = sceneGraph.animations();
	if (animationIds.empty()) {
		animationIds.push_back("Idle");
	}
	const core::String defaultAnim = animationIds[0];
	wrapBool(stream.writeString(defaultAnim.c_str(), true))
	wrapBool(stream.writeInt32(1))
	wrapBool(stream.writeString(stringProperty(&root, "basetemplate")))
	wrapBool(stream.writeBool(boolProperty(&root, "static", false)))
	if (childCount != 1 || sceneGraph.node(children[0]).name() != "Controller") {
		// add controller node (see VXAFormat)
		wrapBool(stream.writeString("Controller", true))
		wrapBool(stream.writeString("", true))

		wrapBool(saveNodeProperties(nullptr, stream))

		wrapBool(stream.writeInt32(childCount))
	}
	for (int child : children) {
		const voxelformat::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, filename, stream))
	}
	const core::String &basePath = core::string::extractPath(filename);
	const core::String &baseName = core::string::extractFilename(filename);
	// TODO: we only support one animation for the transforms at the moment
	for (const core::String &id : animationIds) {
		const core::String &vxaFilename = core::string::format("%s.%s.vxa", baseName.c_str(), id.c_str());
		const core::String &vxaPath = core::string::path(basePath, vxaFilename);
		io::FilePtr outputFile = io::filesystem()->open(vxaPath, io::FileMode::SysWrite);
		if (!outputFile) {
			Log::error("Failed to open %s for writing", vxaPath.c_str());
			return false;
		}
		io::FileStream wstream(outputFile);
		wrapBool(saveVXA(sceneGraph, vxaPath, wstream, id))
	}
	return true;
}

bool VXRFormat::loadChildVXM(const core::String& vxmPath, SceneGraph &sceneGraph, SceneGraphNode &node, int version) {
	const io::FilePtr& file = io::filesystem()->open(vxmPath);
	if (!file->validHandle()) {
		Log::error("Could not open file '%s'", vxmPath.c_str());
		return false;
	}
	io::FileStream stream(file);
	VXMFormat f;
	SceneGraph childSceneGraph;
	if (!f.loadGroups(vxmPath, stream, childSceneGraph)) {
		Log::error("Failed to load '%s'", vxmPath.c_str());
		return false;
	}
	const int modelCount = (int)childSceneGraph.size(SceneGraphNodeType::Model);
	if (modelCount < 1) {
		Log::error("No models found in vxm file: %i", modelCount);
		return false;
	}
	Log::debug("Found %i layers in vxm", modelCount);

	SceneGraphNode* childModelNode = childSceneGraph[0];
	core_assert_always(childModelNode != nullptr);
	childModelNode->releaseOwnership();

	const core::String nodeName = node.name();
	voxelformat::copyNode(*childModelNode, node, false);
	node.setVolume(childModelNode->volume(), true);
	// restore old name
	node.setName(nodeName);

	// TODO: support loading all layers
#if 0
	for (int i = 1; i < modelCount; ++i) {
		SceneGraphNode child;
		SceneGraphNode &src = *childSceneGraph[i];
		copyNode(src, child, false);
		src.releaseOwnership();
		child.setVolume(src.volume(), true);
		// TODO: the node instance is not yet added to the scene graph - and thus doesn't have a parent yet
		sceneGraph.emplace(core::move(child), node.parent());
	}
#endif

	return true;
}

static const InterpolationType interpolationTypes[]{
	InterpolationType::Instant,		 InterpolationType::Linear,			InterpolationType::QuadEaseIn,
	InterpolationType::QuadEaseOut,	 InterpolationType::QuadEaseInOut,	InterpolationType::CubicEaseIn,
	InterpolationType::CubicEaseOut, InterpolationType::CubicEaseInOut,
};

bool VXRFormat::importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	SceneGraphNode node(SceneGraphNodeType::Model);
	char nodeId[1024];
	wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
	node.setName(nodeId);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
	uint32_t animCnt;
	wrap(stream.readUInt32(animCnt))
	char animationId[1024];
	wrapBool(stream.readString(sizeof(animationId), animationId, true))
	node.setProperty("animationid", animationId);
	sceneGraph.addAnimation(animationId);
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		SceneGraphKeyFrame& keyFrame = node.keyFrame(i);
		uint32_t frame;
		wrap(stream.readUInt32(frame)) // frame index
		keyFrame.frameIdx = frame;
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (interpolation < 0 || interpolation >= lengthof(interpolationTypes)) {
			keyFrame.interpolation = InterpolationType::Linear;
			Log::warn("Could not find a supported easing type for %i", interpolation);
		} else {
			keyFrame.interpolation = interpolationTypes[interpolation];
		}
		if (version > 1) {
			keyFrame.longRotation = stream.readBool();
		}
		SceneGraphTransform &transform = keyFrame.transform();
		transform.setPivot(glm::vec3(0.5f));

		glm::quat localOrientation;
		glm::vec3 localTranslation{0.0f};
		float localScale = 1.0f;

		glm::quat ignoredOrientation;
		glm::vec3 ignoredTranslation{0.0f};
		float ignoredScale = 1.0f;

		wrap(stream.readFloat(localTranslation.x))
		//nodeFrame.transform.position.x *= -1.0f;
		wrap(stream.readFloat(localTranslation.y))
		wrap(stream.readFloat(localTranslation.z))
		transform.setWorldTranslation(localTranslation);
		if (version >= 3) {
			wrap(stream.readFloat(ignoredTranslation.x))
			wrap(stream.readFloat(ignoredTranslation.y))
			wrap(stream.readFloat(ignoredTranslation.z))
		}
		if (version == 1) {
			float rotationx = 0.0f;
			float rotationy = 0.0f;
			float rotationz = 0.0f;
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			localOrientation = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			ignoredOrientation = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
		} else {
			wrap(stream.readFloat(localOrientation.x))
			wrap(stream.readFloat(localOrientation.y))
			wrap(stream.readFloat(localOrientation.z))
			wrap(stream.readFloat(localOrientation.w))
			wrap(stream.readFloat(ignoredOrientation.x))
			wrap(stream.readFloat(ignoredOrientation.y))
			wrap(stream.readFloat(ignoredOrientation.z))
			wrap(stream.readFloat(ignoredOrientation.w))
		}
		wrap(stream.readFloat(localScale))
		if (version >= 3) {
			wrap(stream.readFloat(ignoredScale))
		}
		transform.setLocalScale(localScale);
		transform.setLocalTranslation(localTranslation);
		transform.setLocalOrientation(localOrientation);
	}
	int32_t children;
	wrap(stream.readInt32(children))
	const int modelNode = sceneGraph.emplace(core::move(node), parent);
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, modelNode))
	}
	return true;
}

// the positions that were part of the previous vxr versions are now in vxa
bool VXRFormat::importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	SceneGraphNode node(SceneGraphNodeType::Model);
	char id[1024];
	wrapBool(stream.readString(sizeof(id), id, true))
	char filename[1024];
	wrapBool(stream.readString(sizeof(filename), filename, true))
	if (filename[0] != '\0') {
		const core::String modelPath = core::string::path(core::string::extractPath(vxmPath), filename);
		if (!loadChildVXM(modelPath, sceneGraph, node, version)) {
			Log::warn("Failed to attach model for id '%s' with filename %s (%s)", id, filename, modelPath.c_str());
		}
	}
	if (node.volume() == nullptr) {
		node = SceneGraphNode(SceneGraphNodeType::Group);
	}
	node.setName(id);
	node.setProperty("id", id);
	node.setProperty("filename", filename);
	if (version > 4) {
		if (version >= 9) {
			node.setProperty("collidable", stream.readBool());
			node.setProperty("decorative", stream.readBool());
		}
		if (version >= 6) {
			uint32_t color;
			wrap(stream.readUInt32(color))
			node.setColor(core::RGBA(color));
			node.setProperty("favorite", stream.readBool());
			node.setProperty("visible", stream.readBool());
		}
		node.setProperty("mirror x axis", stream.readBool());
		node.setProperty("mirror y axis", stream.readBool());
		node.setProperty("mirror z axis", stream.readBool());
		node.setProperty("preview mirror x axis", stream.readBool());
		node.setProperty("preview mirror y axis", stream.readBool());
		node.setProperty("preview mirror z axis", stream.readBool());
		node.setProperty("ikAnchor", stream.readBool());
		if (version >= 9) {
			char effectorId[1024];
			wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
			node.setProperty("ikEffectorId", effectorId);
			node.setProperty("ikConstraintsVisible", stream.readBool());
			float rollmin;
			wrap(stream.readFloat(rollmin))
			node.setProperty("ikRollMin", core::string::format("%f", rollmin));
			float rollmax;
			wrap(stream.readFloat(rollmax))
			node.setProperty("ikRollMax", core::string::format("%f", rollmax));
			int32_t inverseKinematicsConstraints;
			wrap(stream.readInt32(inverseKinematicsConstraints))
			for (int32_t i = 0; i < inverseKinematicsConstraints; ++i) {
				float inverseKinematicsX, inverseKinematicsY, inverseKinematicsRadius;
				wrap(stream.readFloat(inverseKinematicsX))
				wrap(stream.readFloat(inverseKinematicsY))
				wrap(stream.readFloat(inverseKinematicsRadius))
			}
		} else {
			node.setProperty("pitch constraint", stream.readBool());
			float pitchConstraintMin;
			wrap(stream.readFloat(pitchConstraintMin))
			node.setProperty("pitch constraint min", core::string::format("%f", pitchConstraintMin));
			float pitchConstraintMax;
			wrap(stream.readFloat(pitchConstraintMax))
			node.setProperty("pitch constraint max", core::string::format("%f", pitchConstraintMax));
			stream.readBool(); /* y counter clock wise allowed */
			stream.readBool(); /* y clock wise allowed */
			stream.readBool(); /* z counter clock wise allowed */
			stream.readBool(); /* z clock wise allowed */
		}
	}
	const int nodeId = sceneGraph.emplace(core::move(node), parent);
	if (version >= 4) {
		int32_t children = 0;
		wrap(stream.readInt32(children))
		for (int32_t i = 0; i < children; ++i) {
			wrapBool(importChild(vxmPath, stream, sceneGraph, version, nodeId != -1 ? nodeId : parent))
		}
	}
	return true;
}

image::ImagePtr VXRFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	const core::String imageName = filename + ".png";
	return image::loadImage(imageName, false);
}

bool VXRFormat::loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version) {
	uint32_t childAndModelCount;
	wrap(stream.readUInt32(childAndModelCount))
	uint32_t children = 0;
	wrap(stream.readUInt32(children))
	const int rootNodeId = sceneGraph.root().id();
	for (uint32_t i = 0; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, rootNodeId))
	}
	int32_t modelCount;
	wrap(stream.readInt32(modelCount))
	for (int32_t i = 0; i < modelCount; ++i) {
		char nodeId[1024];
		wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
		SceneGraphNode* node = sceneGraph.findNodeByName(nodeId);
		if (node == nullptr || node->type() != SceneGraphNodeType::Model) {
			Log::error("Can't find referenced model node %s", nodeId);
			return false;
		}
		char vxmFilename[1024];
		wrapBool(stream.readString(sizeof(vxmFilename), vxmFilename, true))
		if (vxmFilename[0] != '\0') {
			const core::String modelPath = core::string::path(core::string::extractPath(filename), vxmFilename);
			if (!loadChildVXM(modelPath, sceneGraph, *node, version)) {
				Log::warn("Failed to attach model for %s with filename %s", nodeId, modelPath.c_str());
			}
		}
	}
	return true;
}

bool VXRFormat::handleVersion8AndLater(io::SeekableReadStream& stream, SceneGraphNode &node) {
	char baseTemplate[1024];
	wrapBool(stream.readString(sizeof(baseTemplate), baseTemplate, true))
	node.setProperty("basetemplate", baseTemplate);
	const bool isStatic = stream.readBool();
	node.setProperty("static", isStatic);
	if (isStatic) {
		int32_t lodLevels;
		wrap(stream.readInt32(lodLevels))
		for (int32_t i = 0 ; i < lodLevels; ++i) {
			uint32_t dummy;
			wrap(stream.readUInt32(dummy))
			wrap(stream.readUInt32(dummy))
			uint32_t diffuseTexZipped;
			wrap(stream.readUInt32(diffuseTexZipped))
			stream.skip(diffuseTexZipped);
			const bool hasEmissive = stream.readBool();
			if (hasEmissive) {
				uint32_t emissiveTexZipped;
				wrap(stream.readUInt32(emissiveTexZipped))
				stream.skip(emissiveTexZipped);
			}
			int32_t quadAmount;
			wrap(stream.readInt32(quadAmount))
			for (int32_t quad = 0; quad < quadAmount; ++quad) {
				for (int v = 0; v < 4; ++v) {
					float dummyFloat;
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
				}
			}
		}
	}
	return true;
}

bool VXRFormat::loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version) {
	const int rootNodeId = sceneGraph.root().id();
	SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);

	// TODO: allow to specify the animation to import... (via cvar?)
	char defaultAnim[1024] = "";
	if (version >= 7) {
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
		rootNode.setProperty("defaultanim", defaultAnim);
	}

	int32_t children = 0;
	wrap(stream.readInt32(children))

	if (version >= 8) {
		handleVersion8AndLater(stream, rootNode);
	}

	Log::debug("Found %i children", children);
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, stream, sceneGraph, version, rootNodeId))
	}

	const core::String& basePath = core::string::extractPath(filename);
	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list(basePath, entities, "*.vxa");
	core::String vxaPath;
	const core::String& baseName = core::string::extractFilename(filename);
	if (defaultAnim[0] != '\0') {
		vxaPath = core::string::path(basePath, core::string::format("%s.%s.vxa", baseName.c_str(), defaultAnim));
		Log::debug("Load the default animation %s", defaultAnim);
	} else if (!entities.empty()) {
		vxaPath = core::string::path(basePath, entities[0].name);
		Log::debug("No default animation set, use the first available vxa: %s", entities[0].name.c_str());
	} else {
		Log::warn("Could not find any vxa file for %s", filename.c_str());
	}

	for (const io::FilesystemEntry& entry : entities) {
		const core::String &animWithExt = entry.name.substr(baseName.size() + 1);
		const core::String &anim = core::string::extractFilename(animWithExt);
		sceneGraph.addAnimation(anim);
	}

	if (!vxaPath.empty() && !loadVXA(sceneGraph, vxaPath)) {
		Log::warn("Failed to load %s", vxaPath.c_str());
	}
	// some files since version 6 still have stuff here
	return true;
}

bool VXRFormat::saveVXA(const SceneGraph& sceneGraph, const core::String &vxaPath, io::SeekableWriteStream& vxaStream, const core::String &animation) {
	VXAFormat f;
	return f.saveGroups(sceneGraph, vxaPath, vxaStream);
}

bool VXRFormat::loadVXA(SceneGraph& sceneGraph, const core::String& vxaPath) {
	Log::debug("Try to load a vxa file: %s", vxaPath.c_str());
	const io::FilePtr& file = io::filesystem()->open(vxaPath);
	if (!file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	VXAFormat format;
	return format.loadGroups(vxaPath, stream, sceneGraph);
}

bool VXRFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, voxel::Palette &) {
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'R') {
		Log::error("Could not load vxr file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version;
	if (magic[3] >= '0' && magic[3] <= '9') {
		version = magic[3] - '0';
	} else {
		Log::error("Invalid version found");
		return false;
	}

	Log::debug("Found vxr version: %i", version);

	sceneGraph.node(0).setProperty("vxrversion", core::string::toString(version));

	if (version < 1 || version > 9) {
		Log::error("Could not load vxr file: Unsupported version found (%i)", version);
		return false;
	}

	if (version <= 3) {
		return loadGroupsVersion3AndEarlier(filename, stream, sceneGraph, version);
	}
	return loadGroupsVersion4AndLater(filename, stream, sceneGraph, version);
}

#undef wrap
#undef wrapBool

}
