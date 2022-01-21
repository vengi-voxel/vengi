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
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXRFormat::saveRecursiveNode(const core::String &name, const voxel::SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeString(node.name(), true))
	const core::String baseName = core::string::stripExtension(filename);
	const core::String finalName = baseName + name + ".vxm";
	wrapBool(stream.writeString(finalName, true))
	VXMFormat f;
	io::FilePtr outputFile = io::filesystem()->open(finalName, io::FileMode::SysWrite);
	if (!outputFile) {
		Log::error("Failed to open %s for writing", finalName.c_str());
		return false;
	}
	io::FileStream wstream(outputFile.get());
	SceneGraph newSceneGraph;
	voxel::SceneGraphNode newNode;
	newNode.setVolume(node.volume(), false);
	newNode.setName(name);
	newNode.setVisible(node.visible());
	newSceneGraph.emplace(core::move(newNode));
	wrapBool(f.saveGroups(newSceneGraph, finalName, wstream))

	wrapBool(stream.writeInt32(0)); // next child count
#if 0
	for (int i = 0; i < childCount; ++i) {
		wrapBool(saveRecursiveNode(i, volumes, filename, stream))
	}
#endif
	return true;
}

bool VXRFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeUInt32(FourCC('V','X','R','4')));

	const int childCount = (int)sceneGraph.size();
	wrapBool(stream.writeInt32(childCount))
	int n = 0;
	for (const SceneGraphNode& node : sceneGraph) {
		core::String name = node.name();
		if (name.empty()) {
			name = core::string::format("%i", n);
		}
		wrapBool(saveRecursiveNode(name, node, filename, stream))
		++n;
	}
	return true;
}

static inline glm::vec4 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot) {
#if 0
	glm::vec3 v(pos);
	v *= 2.0f;
	v -= 1.0f;
	v *= 0.5f * glm::vec3(pivot / 32.0f);
	return glm::vec4(v, 1.0f);
#elif 0
	return glm::floor(mat * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
#else
	return glm::vec4(pos, 1.0f);
#endif
}

static voxel::RawVolume* transformVolume(voxel::SceneGraphNode &targetNode, const voxel::SceneGraphNode &srcNode, int version) {
	const glm::mat4 &mat = targetNode.matrix();
	const voxel::RawVolume *in = srcNode.volume();
	const glm::vec3 &pivotNormalized = srcNode.normalizedPivot();
	const voxel::Region &inRegion = in->region();
	const glm::ivec3 inMins(inRegion.getLowerCorner());
	const glm::ivec3 inMaxs(inRegion.getUpperCorner());
	const glm::vec3 &pivot = pivotNormalized * glm::vec3(inRegion.getDimensionsInVoxels());

	const glm::vec4 outMins(transform(mat, inMins, pivot));
	const glm::vec4 outMaxs(transform(mat, inMaxs, pivot));
	const voxel::Region outRegion(glm::min(outMins, outMaxs), glm::max(outMins, outMaxs));
	voxel::RawVolume *v = new voxel::RawVolume(outRegion);
	for (int z = inMins.z; z <= inMaxs.z; ++z) {
		for (int y = inMins.y; y <= inMaxs.y; ++y) {
			for (int x = inMins.x; x <= inMaxs.x; ++x) {
				const glm::ivec3 vpos(x, y, z);
				const glm::ivec3 tpos(transform(mat, vpos, pivot));
				v->setVoxel(tpos, in->voxel(vpos));
			}
		}
	}
	return v;
}

bool VXRFormat::loadChildVXM(const core::String& vxmPath, voxel::SceneGraphNode &node, int version) {
	const io::FilePtr& file = io::filesystem()->open(vxmPath);
	if (!file->validHandle()) {
		Log::error("Could not open file '%s'", vxmPath.c_str());
		return false;
	}
	io::FileStream stream(file.get());
	VXMFormat f;
	SceneGraph newSceneGraph;
	if (!f.loadGroups(vxmPath, stream, newSceneGraph)) {
		Log::error("Failed to load '%s'", vxmPath.c_str());
		return false;
	}
	const int modelCount = (int)newSceneGraph.size(voxel::SceneGraphNodeType::Model);
	if (modelCount > 1) {
		Log::warn("Unexpected scene graph found in vxm file - only use the first one");
	} else if (modelCount != 1) {
		Log::error("No models found in vxm file: %i", modelCount);
		return false;
	}
	voxel::SceneGraphNode* modelNode = newSceneGraph[0];
	core_assert_always(modelNode != nullptr);

	voxel::RawVolume *v = transformVolume(node, *modelNode, version);
	node.setVolume(v, true);
	node.setVisible(modelNode->visible());
	node.setLocked(modelNode->locked());
	node.addProperties(modelNode->properties());
	return true;
}

bool VXRFormat::importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Model);
	char nodeId[1024];
	wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
	node.setName(nodeId);

	uint32_t animCnt;
	wrap(stream.readUInt32(animCnt))
	char animationId[1024];
	wrapBool(stream.readString(sizeof(animationId), animationId, true))
	node.setProperty("animationid", animationId);
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		uint32_t frame;
		wrap(stream.readUInt32(frame)) // frame index
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (version > 1) {
			stream.readBool(); // rotation ??
		}
		SceneGraphTransform transform;
		glm::vec3 localPosition{0.0f};
		glm::quat localRot{0.0f, 0.0f, 0.0f, 0.0f};
		float localScale = 1.0f;
		wrap(stream.readFloat(transform.position.x))
		wrap(stream.readFloat(transform.position.y))
		wrap(stream.readFloat(transform.position.z))
		if (version >= 3) {
			wrap(stream.readFloat(localPosition.x))
			wrap(stream.readFloat(localPosition.y))
			wrap(stream.readFloat(localPosition.z))
		}
		if (version == 1) {
			float rotationx;
			float rotationy;
			float rotationz;
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			transform.rot = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			localRot = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
		} else {
			wrap(stream.readFloat(transform.rot.x))
			wrap(stream.readFloat(transform.rot.y))
			wrap(stream.readFloat(transform.rot.z))
			wrap(stream.readFloat(transform.rot.w))
			wrap(stream.readFloat(localRot.x))
			wrap(stream.readFloat(localRot.y))
			wrap(stream.readFloat(localRot.z))
			wrap(stream.readFloat(localRot.w))
		}
		wrap(stream.readFloat(transform.scale))
		if (version >= 3) {
			wrap(stream.readFloat(localScale))
		}
		// TODO: only the first frame is supported - as we don't have animation support yet
		if (i == 0u) {
			transform.position /= 32.0f;
			node.setTransform(transform, true);
		}
	}
	int32_t children;
	wrap(stream.readInt32(children))
	const int modelNode = sceneGraph.emplace(core::move(node), parent);
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, modelNode))
	}
	return true;
}

bool VXRFormat::importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Model);
	char id[1024];
	wrapBool(stream.readString(sizeof(id), id, true))
	node.setName(id);
	char filename[1024];
	wrapBool(stream.readString(sizeof(filename), filename, true))
	if (filename[0] != '\0') {
		core::String modelPath = core::string::extractPath(vxmPath);
		if (!modelPath.empty()) {
			modelPath.append("/");
		}
		modelPath.append(filename);
		if (!loadChildVXM(modelPath, node, version)) {
			Log::warn("Failed to attach model for id '%s' with filename %s (%s)", id, filename, modelPath.c_str());
		}
	}
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
			node.setProperty("color", core::Color::toHex(color));
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
		float dummyf;
		if (version >= 9) {
			char effectorId[1024];
			wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
			node.setProperty("effectorId", effectorId);
			node.setProperty("constraints visible", stream.readBool());
			wrap(stream.readFloat(dummyf)) // rollmin
			node.setProperty("rollmin", dummyf);
			wrap(stream.readFloat(dummyf)) // rollmax
			node.setProperty("rollmax", dummyf);
			int32_t constraints;
			wrap(stream.readInt32(constraints))
			for (int32_t i = 0; i < constraints; ++i) {
				wrap(stream.readFloat(dummyf)) // x
				wrap(stream.readFloat(dummyf)) // z
				wrap(stream.readFloat(dummyf)) // radius
			}
		} else {
			stream.readBool(); // ???
			wrap(stream.readFloat(dummyf)) // ???
			wrap(stream.readFloat(dummyf)) // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
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
	uint32_t dummy;
	wrap(stream.readUInt32(dummy))
	uint32_t children = 0;
	wrap(stream.readUInt32(children))
	const int rootNode = sceneGraph.root().id();
	for (uint32_t i = 0; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, rootNode))
	}
	int32_t modelCount;
	wrap(stream.readInt32(modelCount))
	for (int32_t i = 0; i < modelCount; ++i) {
		char nodeId[1024];
		wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
		voxel::SceneGraphNode* node = sceneGraph.findNodeByName(nodeId);
		if (node == nullptr || node->type() != voxel::SceneGraphNodeType::Model) {
			Log::error("Can't find referenced model node %s", nodeId);
			return false;
		}
		char vxmFilename[1024];
		wrapBool(stream.readString(sizeof(vxmFilename), vxmFilename, true))
		if (vxmFilename[0] != '\0') {
			core::String modelPath = core::string::extractPath(filename);
			if (!modelPath.empty()) {
				modelPath.append("/");
			}
			modelPath.append(vxmFilename);
			if (!loadChildVXM(modelPath, *node, version)) {
				Log::warn("Failed to attach model for %s with filename %s", nodeId, modelPath.c_str());
			}
		}
	}
	return true;
}

bool VXRFormat::handleVersion8AndLater(io::SeekableReadStream& stream, voxel::SceneGraphNode &node) {
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
	int32_t children = 0;
	wrap(stream.readInt32(children))

	const int rootNodeId = sceneGraph.root().id();
	voxel::SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);

	if (version >= 8) {
		handleVersion8AndLater(stream, rootNode);
	}

	Log::debug("Found %i children", children);
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, stream, sceneGraph, version, rootNodeId))
	}

	// some files since version 6 still have stuff here
	return true;
}

bool VXRFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
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

	if (version < 1 || version > 9) {
		Log::error("Could not load vxr file: Unsupported version found (%i)", version);
		return false;
	}

	const int rootNodeId = sceneGraph.root().id();
	voxel::SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);

	if (version >= 7) {
		char defaultAnim[1024];
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
		rootNode.setProperty("defaultanim", defaultAnim);
	}
	if (version <= 3) {
		return loadGroupsVersion3AndEarlier(filename, stream, sceneGraph, version);
	}
	return loadGroupsVersion4AndLater(filename, stream, sceneGraph, version);
}

#undef wrap
#undef wrapBool

}
