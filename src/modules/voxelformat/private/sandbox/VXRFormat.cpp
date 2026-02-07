/**
 * @file
 */

#include "VXRFormat.h"
#include "VXAFormat.h"
#include "VXMFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "io/StreamUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load vxr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool VXRFormat::saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								  const core::String &filename, const io::ArchivePtr &archive,
								  io::SeekableWriteStream &stream, const SaveContext &ctx) {
	core::String name = node.name();
	if (name.empty()) {
		name = core::String::format("%i", node.id());
	}
	wrapBool(stream.writeString(node.name(), true))
	if (node.isAnyModelNode()) {
		const core::String baseName = core::string::stripExtension(core::string::extractFilename(filename));
		const core::String finalName = baseName + name + ".vxm";
		wrapBool(stream.writeString(finalName, true))
		const core::String fullPath = core::string::stripExtension(filename) + name + ".vxm";
		scenegraph::SceneGraph newSceneGraph;
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		copyNode(node, newNode, false);
		if (node.isReferenceNode()) {
			newNode.setVolume(sceneGraph.resolveVolume(node));
		}
		newSceneGraph.emplace(core::move(newNode));
		VXMFormat f;
		wrapBool(f.save(newSceneGraph, fullPath, archive, ctx))
		Log::debug("Saved the model to %s", fullPath.c_str());
	} else {
		wrapBool(stream.writeString("", true))
	}

	wrapBool(saveNodeProperties(&node, stream))

	const int32_t childCount = (int32_t)node.children().size();
	wrapBool(stream.writeInt32(childCount));
	for (int child : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, cnode, filename, archive, stream, ctx))
	}
	return true;
}

bool VXRFormat::saveNodeProperties(const scenegraph::SceneGraphNode *node, io::SeekableWriteStream &stream) {
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
	if (node != nullptr) {
		if (const scenegraph::IKConstraint *ikConstraint = node->ikConstraint()) {
			wrapBool(stream.writeBool(ikConstraint->anchor))
			// resolve the effector node name for serialization
			const core::String effectorName = stringProperty(node, "ikEffectorId");
			wrapBool(stream.writeString(effectorName, true))
			wrapBool(stream.writeBool(ikConstraint->visible))
			wrapBool(stream.writeFloat(ikConstraint->rollMin))
			wrapBool(stream.writeFloat(ikConstraint->rollMax))
			const int ikConstraintAmount = (int)ikConstraint->swingLimits.size();
			wrapBool(stream.writeUInt32(ikConstraintAmount))
			for (int i = 0; i < ikConstraintAmount; ++i) {
				wrapBool(stream.writeFloat(ikConstraint->swingLimits[i].center.x))
				wrapBool(stream.writeFloat(ikConstraint->swingLimits[i].center.y))
				wrapBool(stream.writeFloat(ikConstraint->swingLimits[i].radius))
			}
		} else {
			wrapBool(stream.writeBool(false))
			wrapBool(stream.writeString("", true))
			wrapBool(stream.writeBool(true))
			wrapBool(stream.writeFloat(0.0f))
			wrapBool(stream.writeFloat(glm::two_pi<float>()))
			wrapBool(stream.writeUInt32(0))
		}
	} else {
		wrapBool(stream.writeBool(false))
		wrapBool(stream.writeString("", true))
		wrapBool(stream.writeBool(true))
		wrapBool(stream.writeFloat(0.0f))
		wrapBool(stream.writeFloat(glm::two_pi<float>()))
		wrapBool(stream.writeUInt32(0))
	}
	return true;
}

bool VXRFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
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
		Log::error("Empty scene graph - can't save vxr");
		return false;
	}
	wrapBool(stream->writeUInt32(FourCC('V', 'X', 'R', '9')))
	scenegraph::SceneGraphAnimationIds animationIds = sceneGraph.animations();
	if (animationIds.empty()) {
		animationIds.push_back("Idle");
	}
	const core::String defaultAnim = animationIds[0];
	wrapBool(stream->writeString(defaultAnim.c_str(), true))
	wrapBool(stream->writeInt32(1))
	wrapBool(stream->writeString(stringProperty(&root, "basetemplate"), true))
	wrapBool(stream->writeBool(boolProperty(&root, "static", false)))
	if (childCount != 1 || sceneGraph.node(children[0]).name() != SANDBOX_CONTROLLER_NODE) {
		// add controller node (see VXAFormat)
		wrapBool(stream->writeString(SANDBOX_CONTROLLER_NODE, true))
		wrapBool(stream->writeString("", true))

		wrapBool(saveNodeProperties(nullptr, *stream))
		Log::debug("add controller node with %i children", childCount);
		wrapBool(stream->writeInt32(childCount))
	}
	for (int child : children) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, filename, archive, *stream, ctx))
	}
	const core::String &basePath = core::string::extractDir(filename);
	const core::String &baseName = core::string::extractFilename(filename);
	for (const core::String &id : animationIds) {
		const core::String &vxaFilename = core::String::format("%s.%s.vxa", baseName.c_str(), id.c_str());
		const core::String &vxaPath = core::string::path(basePath, vxaFilename);
		wrapBool(saveVXA(sceneGraph, vxaPath, archive, id, ctx))
	}
	return true;
}

bool VXRFormat::loadChildVXM(const core::String &vxmPath, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
							 scenegraph::SceneGraphNode &node, int version, const LoadContext &ctx) {
	VXMFormat f;
	scenegraph::SceneGraph childSceneGraph;
	if (!f.load(vxmPath, archive, childSceneGraph, ctx)) {
		Log::error("Failed to load '%s'", vxmPath.c_str());
		return false;
	}
	const int modelCount = (int)childSceneGraph.size(scenegraph::SceneGraphNodeType::Model);
	if (modelCount < 1) {
		Log::error("No models found in vxm file: %i", modelCount);
		return false;
	}
	Log::debug("Found %i models in vxm", modelCount);

	scenegraph::SceneGraphNode &childModelNode = *childSceneGraph.beginModel();
	childModelNode.releaseOwnership();

	const core::String nodeName = node.name();
	const glm::vec3 pivot = childModelNode.pivot();
	scenegraph::copyNode(childModelNode, node, false, version >= 3);
	node.setVolume(childModelNode.volume(), true);
	node.setPivot(pivot);
	// restore old name
	node.setName(nodeName);

	// TODO: VOXELFORMAT: support loading all models
#if 0
	for (int i = 1; i < modelCount; ++i) {
		SceneGraphNode child;
		SceneGraphNode &src = *childSceneGraph[i];
		copyNode(src, child, false);
		src.releaseOwnership();
		child.setVolume(src.volume(), true);
		// TODO: VOXELFORMAT: the node instance is not yet added to the scene graph - and thus doesn't have a parent yet
		sceneGraph.emplace(core::move(child), node.parent());
	}
#endif

	return true;
}

static const scenegraph::InterpolationType interpolationTypes[]{
	scenegraph::InterpolationType::Instant,		  scenegraph::InterpolationType::Linear,
	scenegraph::InterpolationType::QuadEaseIn,	  scenegraph::InterpolationType::QuadEaseOut,
	scenegraph::InterpolationType::QuadEaseInOut, scenegraph::InterpolationType::CubicEaseIn,
	scenegraph::InterpolationType::CubicEaseOut,  scenegraph::InterpolationType::CubicEaseInOut,
};

bool VXRFormat::importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream &stream,
											  scenegraph::SceneGraph &sceneGraph, int version, int parent,
											  const LoadContext &ctx) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	char nodeId[1024];
	wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
	Log::debug("load node %s", nodeId);
	node.setName(nodeId);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
	uint32_t animCnt;
	wrap(stream.readUInt32(animCnt))
	char animationId[1024];
	wrapBool(stream.readString(sizeof(animationId), animationId, true))
	node.setProperty("animationid", animationId);
	sceneGraph.addAnimation(animationId);
	node.setAnimation(animationId);
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(i);
		int32_t frame;
		wrap(stream.readInt32(frame)) // frame index
		keyFrame.frameIdx = frame;
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (interpolation < 0 || interpolation >= lengthof(interpolationTypes)) {
			keyFrame.interpolation = scenegraph::InterpolationType::Linear;
			Log::warn("Could not find a supported easing type for %i", interpolation);
		} else {
			keyFrame.interpolation = interpolationTypes[interpolation];
		}
		if (version > 1) {
			keyFrame.longRotation = stream.readBool();
		}
		scenegraph::SceneGraphTransform &transform = keyFrame.transform();

		glm::quat localOrientation;
		glm::vec3 localTranslation{0.0f};
		float localScale = 1.0f;

		glm::quat ignoredOrientation;
		glm::vec3 ignoredTranslation{0.0f};
		float ignoredScale = 1.0f;

		wrapBool(io::readVec3(stream, localTranslation))
		localTranslation.x *= -1.0f; // version 2 needed this
		if (version >= 3) {
			wrapBool(io::readVec3(stream, ignoredTranslation))
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
			wrapBool(io::readQuat(stream, localOrientation))
			wrapBool(io::readQuat(stream, ignoredOrientation))
		}
		wrap(stream.readFloat(localScale))
		if (version >= 3) {
			wrap(stream.readFloat(ignoredScale))
		}
		transform.setLocalScale(glm::vec3(localScale));
		transform.setLocalTranslation(localTranslation);
		transform.setLocalOrientation(localOrientation);
	}
	int32_t children;
	wrap(stream.readInt32(children))
	const int modelNode = sceneGraph.emplace(core::move(node), parent);
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, modelNode, ctx))
	}
	return true;
}

// the positions that were part of the previous vxr versions are now in vxa
bool VXRFormat::importChild(const core::String &vxmPath, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
							scenegraph::SceneGraph &sceneGraph, int version, int parent, const LoadContext &ctx) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	char id[1024];
	wrapBool(stream.readString(sizeof(id), id, true))
	Log::debug("load node %s", id);
	char filename[1024];
	wrapBool(stream.readString(sizeof(filename), filename, true))
	if (filename[0] != '\0') {
		Log::debug("load vxm %s", filename);
		const core::String modelPath = core::string::path(core::string::extractDir(vxmPath), filename);
		if (!loadChildVXM(modelPath, archive, sceneGraph, node, version, ctx)) {
			Log::warn("Failed to attach model for id '%s' with filename %s (%s)", id, filename, modelPath.c_str());
		}
	}
	if (node.volume() == nullptr) {
		node = scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Group);
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
			node.setColor(color::RGBA(color));
			node.setProperty("favorite", stream.readBool());
			node.setProperty("visible", stream.readBool());
		}
		node.setProperty("mirror x axis", stream.readBool());
		node.setProperty("mirror y axis", stream.readBool());
		node.setProperty("mirror z axis", stream.readBool());
		node.setProperty("preview mirror x axis", stream.readBool());
		node.setProperty("preview mirror y axis", stream.readBool());
		node.setProperty("preview mirror z axis", stream.readBool());
		scenegraph::IKConstraint ikConstraint;
		ikConstraint.anchor = stream.readBool();
		if (version >= 9) {
			char effectorId[1024];
			wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
			node.setProperty("ikEffectorId", effectorId);
			// the effector node might not exist yet - we resolve after the full scene graph is loaded
			scenegraph::SceneGraphNode *effectorNode = sceneGraph.findNodeByName(effectorId);
			ikConstraint.effectorNodeId = effectorNode != nullptr ? effectorNode->id() : InvalidNodeId;
			ikConstraint.visible = stream.readBool();
			wrap(stream.readFloat(ikConstraint.rollMin))
			wrap(stream.readFloat(ikConstraint.rollMax))
			int32_t inverseKinematicsConstraints;
			wrap(stream.readInt32(inverseKinematicsConstraints))
			ikConstraint.swingLimits.resize(inverseKinematicsConstraints);
			for (int32_t i = 0; i < inverseKinematicsConstraints; ++i) {
				scenegraph::IKConstraint::RadiusConstraint &constraint = ikConstraint.swingLimits[i];
				wrap(stream.readFloat(constraint.center.x))
				wrap(stream.readFloat(constraint.center.y))
				wrap(stream.readFloat(constraint.radius))
			}
		} else {
			const bool pitchConstraintEnabled = stream.readBool();
			float pitchConstraintMin;
			wrap(stream.readFloat(pitchConstraintMin))
			float pitchConstraintMax;
			wrap(stream.readFloat(pitchConstraintMax))
			stream.readBool(); /* y counter clock wise allowed */
			stream.readBool(); /* y clock wise allowed */
			stream.readBool(); /* z counter clock wise allowed */
			stream.readBool(); /* z clock wise allowed */
			if (pitchConstraintEnabled) {
				ikConstraint.rollMin = pitchConstraintMin;
				ikConstraint.rollMax = pitchConstraintMax;
			}
		}
		node.setIkConstraint(ikConstraint);
	}
	Log::debug("Add node %s with parent %i", id, parent);
	const int nodeId = sceneGraph.emplace(core::move(node), parent);
	if (version >= 4) {
		int32_t children = 0;
		wrap(stream.readInt32(children))
		for (int32_t i = 0; i < children; ++i) {
			wrapBool(importChild(vxmPath, archive, stream, sceneGraph, version, nodeId != InvalidNodeId ? nodeId : parent, ctx))
		}
	}
	return true;
}

image::ImagePtr VXRFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	const core::String image = filename + ".png";
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(image));
	if (!stream) {
		Log::error("Could not load file %s", image.c_str());
		return image::ImagePtr();
	}
	return image::loadImage(image, *stream, stream->size());
}

bool VXRFormat::loadGroupsVersion3AndEarlier(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
											 scenegraph::SceneGraph &sceneGraph, int version, const LoadContext &ctx) {
	uint32_t childAndModelCount;
	wrap(stream.readUInt32(childAndModelCount))
	uint32_t children = 0;
	wrap(stream.readUInt32(children))
	const int rootNodeId = sceneGraph.root().id();
	for (uint32_t i = 0; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, rootNodeId, ctx))
	}
	int32_t modelCount;
	wrap(stream.readInt32(modelCount))
	for (int32_t i = 0; i < modelCount; ++i) {
		char nodeId[1024];
		wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
		scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(nodeId);
		if (node == nullptr || node->type() != scenegraph::SceneGraphNodeType::Model) {
			Log::error("Can't find referenced model node %s", nodeId);
			return false;
		}
		char vxmFilename[1024];
		wrapBool(stream.readString(sizeof(vxmFilename), vxmFilename, true))
		if (vxmFilename[0] != '\0') {
			const core::String modelPath = core::string::path(core::string::extractDir(filename), vxmFilename);
			if (!loadChildVXM(modelPath, archive, sceneGraph, *node, version, ctx)) {
				Log::warn("Failed to attach model for %s with filename %s", nodeId, modelPath.c_str());
			}
		}
	}

	return true;
}

bool VXRFormat::handleVersion8AndLater(io::SeekableReadStream &stream, scenegraph::SceneGraphNode &node,
									   const LoadContext &ctx) {
	char baseTemplate[1024];
	wrapBool(stream.readString(sizeof(baseTemplate), baseTemplate, true))
	node.setProperty("basetemplate", baseTemplate);
	const bool isStatic = stream.readBool();
	node.setProperty("static", isStatic);
	if (isStatic) {
		int32_t lodLevels;
		wrap(stream.readInt32(lodLevels))
		for (int32_t i = 0; i < lodLevels; ++i) {
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

bool VXRFormat::loadGroupsVersion4AndLater(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
										   scenegraph::SceneGraph &sceneGraph, int version, const LoadContext &ctx) {
	const int rootNodeId = sceneGraph.root().id();
	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);

	char defaultAnim[1024] = "";
	if (version >= 7) {
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
		rootNode.setProperty("defaultanim", defaultAnim);
	}

	int32_t children = 0;
	wrap(stream.readInt32(children))

	if (version >= 8) {
		wrapBool(handleVersion8AndLater(stream, rootNode, ctx));
	}

	Log::debug("Found %i children", children);
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, archive, stream, sceneGraph, version, rootNodeId, ctx))
	}

	const core::String &basePath = core::string::extractDir(filename);
	const core::String &baseName = core::string::extractFilename(filename);
	io::ArchiveFiles entities;
	archive->list(basePath, entities, "*.vxa");

	for (const io::FilesystemEntry &entry : entities) {
		Log::debug("Found vxa: %s for name %s", entry.name.c_str(), baseName.c_str());
		if (!core::string::startsWith(entry.name, baseName)) {
			Log::debug("Skip vxa: %s", entry.name.c_str());
			continue;
		}
		Log::debug("Load vxa: %s", entry.name.c_str());
		const core::String &vxaPath = core::string::path(basePath, entry.name);
		if (!loadVXA(sceneGraph, vxaPath, archive, ctx)) {
			Log::warn("Failed to load %s", vxaPath.c_str());
		}
	}

	Log::debug("Default animation is: '%s", defaultAnim);
	sceneGraph.setAnimation(defaultAnim);

	// some files since version 6 still have stuff here
	return true;
}

bool VXRFormat::saveVXA(const scenegraph::SceneGraph &sceneGraph, const core::String &vxaPath,
						const io::ArchivePtr &archive, const core::String &animation, const SaveContext &ctx) {
	VXAFormat f;
	return f.save(sceneGraph, vxaPath, archive, ctx);
}

bool VXRFormat::loadVXA(scenegraph::SceneGraph &sceneGraph, const core::String &vxaPath, const io::ArchivePtr &archive, const LoadContext &ctx) {
	Log::debug("Try to load a vxa file: %s", vxaPath.c_str());
	VXAFormat format;
	return format.load(vxaPath, archive, sceneGraph, ctx);
}

bool VXRFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t magic[4];
	wrap(stream->readUInt8(magic[0]))
	wrap(stream->readUInt8(magic[1]))
	wrap(stream->readUInt8(magic[2]))
	wrap(stream->readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'R') {
		Log::error("Could not load vxr file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
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
		return loadGroupsVersion3AndEarlier(filename, archive, *stream, sceneGraph, version, ctx);
	}
	return loadGroupsVersion4AndLater(filename, archive, *stream, sceneGraph, version, ctx);
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
