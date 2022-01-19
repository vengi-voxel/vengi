/**
 * @file
 */

#include "VXRFormat.h"
#include "core/Assert.h"
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
	newNode.setPivot(node.pivot());
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

static inline glm::vec4 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec4 &pivot) {
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

static voxel::RawVolume* transformVolume(const glm::mat4 &mat, const voxel::RawVolume *in) {
	const voxel::Region &inRegion = in->region();
	const glm::ivec3 inMins(inRegion.getLowerCorner());
	const glm::ivec3 inMaxs(inRegion.getUpperCorner());

	const glm::ivec3 size = inRegion.getDimensionsInCells();
	const glm::vec4 pivot((float)size.x / 2.0f + 0.5f, (float)size.y / 2.0f + 0.5f, (float)size.z / 2.0f + 0.5f, 0.0f);
	const glm::vec4 outMins(transform(mat, inMins, pivot));
	const glm::vec4 outMaxs(transform(mat, inMaxs, pivot));
	const voxel::Region outRegion(glm::min(outMins, outMaxs), glm::max(outMins, outMaxs));
	voxel::RawVolume *v = new voxel::RawVolume(outRegion);
	for (int z = inMins.z; z <= inMaxs.z; ++z) {
		for (int y = inMins.y; y <= inMaxs.y; ++y) {
			for (int x = inMins.x; x <= inMaxs.x; ++x) {
				const glm::ivec3 vpos(x, y, z);
				v->setVoxel(transform(mat, vpos, pivot), in->voxel(vpos));
			}
		}
	}
	return v;
}

int VXRFormat::loadChildVXM(const core::String& vxmPath, SceneGraph& sceneGraph, int parent, voxel::SceneGraphNode &node) {
	const io::FilePtr& file = io::filesystem()->open(vxmPath);
	if (!file->validHandle()) {
		Log::error("Could not open file '%s'", vxmPath.c_str());
		return -1;
	}
	io::FileStream stream(file.get());
	VXMFormat f;
	SceneGraph newSceneGraph;
	if (!f.loadGroups(vxmPath, stream, newSceneGraph)) {
		Log::error("Failed to load '%s'", vxmPath.c_str());
		return -1;
	}
	const int modelCount = (int)newSceneGraph.size(voxel::SceneGraphNodeType::Model);
	if (modelCount > 1) {
		Log::warn("Unexpected scene graph found in vxm file - only use the first one");
	} else if (modelCount != 1) {
		Log::error("No models found in vxm file: %i", modelCount);
		return -1;
	}
	voxel::SceneGraphNode* modelNode = newSceneGraph[0];
	core_assert_always(modelNode != nullptr);

	voxel::RawVolume *v = transformVolume(node.matrix(), modelNode->volume());
	node.setVolume(v, true);
	node.setVisible(modelNode->visible());
	node.setLocked(modelNode->locked());
	node.addProperties(modelNode->properties());
	return sceneGraph.emplace(core::move(node), parent);
}

bool VXRFormat::importChildOld(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, uint32_t version, int parent) {
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
		glm::vec3 position;
		glm::vec3 localPosition(0.0f);
		glm::quat rot;
		glm::quat localRot;
		float scale;
		float localScale = 1.0f;
		wrap(stream.readFloat(position.x))
		wrap(stream.readFloat(position.y))
		wrap(stream.readFloat(position.z))
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
			rot = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			localRot = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
		} else {
			wrap(stream.readFloat(rot.x))
			wrap(stream.readFloat(rot.y))
			wrap(stream.readFloat(rot.z))
			wrap(stream.readFloat(rot.w))
			wrap(stream.readFloat(localRot.x))
			wrap(stream.readFloat(localRot.y))
			wrap(stream.readFloat(localRot.z))
			wrap(stream.readFloat(localRot.w))
		}
		wrap(stream.readFloat(scale))
		if (version >= 3) {
			wrap(stream.readFloat(localScale))
		}
		//node.setMatrix(glm::scale(glm::translate(glm::mat4_cast(rot), position / 32.0f), glm::vec3(scale)));
		//node.setMatrix(glm::scale(glm::translate(glm::mat4(1.0f), position / 32.0f), glm::vec3(scale)));
		//node.setMatrix(glm::translate(glm::mat4(1.0f), position / 32.0f));
		node.setMatrix(glm::translate(glm::mat4(1.0f), position));
	}
	int32_t children;
	wrap(stream.readInt32(children))
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildOld(filename, stream, sceneGraph, version, parent))
	}
	sceneGraph.emplace(core::move(node), parent);
	return true;
}

static void addProperty(voxel::SceneGraphNode* node, const char *name, float value) {
	if (node == nullptr) {
		return;
	}
	node->setProperty(name, core::string::toString(value));
}

static void addProperty(voxel::SceneGraphNode* node, const char *name, bool value) {
	if (node == nullptr) {
		return;
	}
	node->setProperty(name, value ? "true" : "false");
}

static void addProperty(voxel::SceneGraphNode* node, const char *name, const char *value) {
	if (node == nullptr) {
		return;
	}
	node->setProperty(name, value);
}

bool VXRFormat::importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, uint32_t version, int parent) {
	voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Model);
	int nodeId = -1;
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
		nodeId = loadChildVXM(modelPath, sceneGraph, parent, node);
		if (nodeId == -1) {
			Log::warn("Failed to attach model for id '%s' with filename %s (%s)", id, filename, modelPath.c_str());
		}
	} else {
		voxel::SceneGraphNode node(voxel::SceneGraphNodeType::Unknown);
		node.setName(id);
		nodeId = sceneGraph.emplace(core::move(node), parent);
	}
	addProperty(&node, "id", id);
	addProperty(&node, "filename", filename);
	if (version > 4) {
		if (version >= 9) {
			addProperty(&node, "collidable", stream.readBool());
			addProperty(&node, "decorative", stream.readBool());
		}
		uint32_t dummy;
		if (version >= 6) {
			wrap(stream.readUInt32(dummy)) // color
			addProperty(&node, "favorite", stream.readBool());
			addProperty(&node, "visible", stream.readBool());
		}
		addProperty(&node, "mirror x axis", stream.readBool());
		addProperty(&node, "mirror y axis", stream.readBool());
		addProperty(&node, "mirror z axis", stream.readBool());
		addProperty(&node, "preview mirror x axis", stream.readBool());
		addProperty(&node, "preview mirror y axis", stream.readBool());
		addProperty(&node, "preview mirror z axis", stream.readBool());
		addProperty(&node, "ikAnchor", stream.readBool());
		float dummyf;
		if (version >= 9) {
			char effectorId[1024];
			wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
			addProperty(&node, "effectorId", effectorId);
			addProperty(&node, "constraints visible", stream.readBool());
			wrap(stream.readFloat(dummyf)) // rollmin
			addProperty(&node, "rollmin", dummyf);
			wrap(stream.readFloat(dummyf)) // rollmax
			addProperty(&node, "rollmax", dummyf);
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

	if (version < 1 || version > 9) {
		Log::error("Could not load vxr file: Unsupported version found (%i)", version);
		return false;
	}

	int parentNodeId;
	{
		voxel::SceneGraphNode groupNode(voxel::SceneGraphNodeType::Group);
		groupNode.setName("VXR");
		parentNodeId = sceneGraph.emplace(core::move(groupNode), sceneGraph.root().id());
	}

	voxel::SceneGraphNode *node = nullptr;
	if (sceneGraph.hasNode(parentNodeId)) {
		node = &sceneGraph.node(parentNodeId);
	}
	if (version >= 7) {
		char defaultAnim[1024];
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
		addProperty(node, "defaultanim", defaultAnim);
	}
	if (version <= 3) {
		uint32_t dummy;
		wrap(stream.readUInt32(dummy))
		uint32_t children = 0;
		wrap(stream.readUInt32(children))
		for (uint32_t i = 0; i < children; ++i) {
			wrapBool(importChildOld(filename, stream, sceneGraph, version, parentNodeId))
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
				if (!loadChildVXM(modelPath, sceneGraph, parentNodeId, *node)) {
					Log::warn("Failed to attach model for %s with filename %s", nodeId, modelPath.c_str());
				}
			}
		}
		return true;
	}

	int32_t children = 0;
	wrap(stream.readInt32(children))

	if (version >= 8) {
		char baseTemplate[1024];
		wrapBool(stream.readString(sizeof(baseTemplate), baseTemplate, true))
		addProperty(node, "basetemplate", baseTemplate);
		const bool isStatic = stream.readBool();
		addProperty(node, "static", isStatic);
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
	}

	Log::debug("Found %i children (%i)", children, (int)sceneGraph.size());
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, stream, sceneGraph, version, parentNodeId))
	}

	// some files since version 6 still have stuff here
	return true;
}

#undef wrap
#undef wrapBool

}
