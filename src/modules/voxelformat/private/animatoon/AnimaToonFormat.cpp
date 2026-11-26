/**
 * @file
 */

#include "AnimaToonFormat.h"
#include "math/Axes.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Base64ReadStream.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "palette/ColorPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "palette/PaletteLookup.h"
#include "json/JSON.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"

namespace voxelformat {


bool AnimaToonFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
									 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	ctx.setProgressText("AnimaToon");
	ctx.setProgress(0.0f);
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const int64_t size = stream->size();
	core::String jsonStr(size, ' ');
	if (!stream->readString((int)jsonStr.size(), jsonStr.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	json::Json json = json::Json::parse(jsonStr);
	core::String sceneName = json.strVal("SceneName", "unknown").c_str();
	Log::debug("Name: %s", sceneName.c_str());

	// there are a few different scenes in animatoon 3.0 - each scene has one definition
	// the definition contains the size of the volumes and the names of the nodes as
	// well as the child/parent relationship. This definition is hard-coded in animatoon
	struct SceneDefinition {
		core::String name;
		math::Vec3i size;
		core::DynamicArray<core::String> nodeNames;
		core::DynamicArray<core::String> parentNames;
	} sceneDefinitions[] = {
		{"Bird", math::Vec3i(40, 30, 60),
			{
				"Body", "Wing1", "Wing2"
			},
			{
				"", "Body", "Body"
			}
		},
		{"Multiped", math::Vec3i(30, 30, 30),
			{
				"Body", "Head", "LegUpper1", "LegLower1", "Foot1", "LegUpper2", "LegLower2",
				"Foot2", "LegUpper3", "LegLower3", "Foot3", "LegUpper4", "LegLower4", "Foot4",
				"LegUpper5", "LegLower5", "Foot5", "LegUpper6", "LegLower6", "Foot6"
			},
			{
				"", "Body", "Body", "LegUpper1", "LegUpper1", "Body", "LegUpper2",
				"LegUpper2", "Body", "LegUpper3", "LegUpper3", "Body", "LegUpper4", "LegUpper4",
				"Body", "LegUpper5", "LegUpper5", "Body", "LegUpper6", "LegUpper6"
			}
		},
		{"Quad_Simple", math::Vec3i(16),
			{
				"Body", "Head", "Front Left Leg", "Front Right Leg", "Back Left Leg", "Back Right Leg"
			},
			{
				"", "Body", "Body", "Body", "Body", "Body"
			}
		},
		{"Quadruped", math::Vec3i(70, 70, 70),
			{
				"Right Back Leg 1", "Right Back Leg 2", "Right Back Leg 3", "Right Back Foot", "Right Back Toe",
				"Left Back Leg 1", "Left Back Leg 2", "Left Back Leg 3", "Left Back Foot", "Left Back Toe",
				"Body Back", "Body 2", "Body 3", "Body 4", "Shoulder", "Neck 1", "Neck 2", "Head",
				"Left Front Leg 1", "Left Front Leg 2", "Left Front Leg 3", "Left Front Foot", "Left Front Toe",
				"Right Front Leg 1", "Right Front Leg 2", "Right Front Leg 3", "Right Front Foot", "Right Front Toe"
			},
			{
				"", "Right Back Leg 1", "Right Back Leg 2", "Right Back Leg 3", "Right Back Foot",
				"", "Left Back Leg 1", "Left Back Leg 2", "Left Back Leg 3", "Left Back Foot",
				"Body 2", "Body 3", "Body 4", "Shoulder", "", "Shoulder", "Neck 1", "Neck 2",
				"Shoulder", "Left Front Leg 1", "Left Front Leg 2", "Left Front Leg 3", "Left Front Foot",
				"Shoulder", "Right Front Leg 1", "Right Front Leg 2", "Right Front Leg 3", "Right Front Foot"
			}
		},
		{"Biped_Full", math::Vec3i(70, 70, 70),
			{
				"Hip", "Body", "Shoulder",
				"Right Arm 1", "Right Arm 2", "Right Hand", "Right Finger", "Right Thumb",
				"Left Arm 1", "Left Arm 2", "Left Hand", "Left Finger", "Left Thumb",
				"Head",
				"Left Leg 1", "Left Leg 2", "Left Foot", "Left Toe",
				"Right Leg 1", "Right Leg 2", "Right Foot", "Right Toe"
			},
			{
				"", "Hip", "Body",
				"Body", "Right Arm 1", "Right Arm 2", "Right Hand", "Right Arm 2",
				"Body", "Left Arm 1", "Left Arm 2", "Left Hand", "Left Arm 2",
				"Body",
				"Body", "Left Leg 1", "Left Leg 2", "Left Foot",
				"Body", "Right Leg 1", "Right Leg 2", "Right Foot"
			}
		},
		{"Biped_FatGuy", math::Vec3i(70, 70, 70),
			{
				"Hip", "Body",
				"Right Arm 1", "Right Arm 2", "Right Hand", "Right Finger", "Right Thumb",
				"Left Arm 1", "Left Arm 2", "Left Hand", "Left Finger", "Left Thumb",
				"Head",
				"Left Leg 1", "Left Leg 2", "Left Foot", "Left Toe",
				"Right Leg 1", "Right Leg 2", "Right Foot", "Right Toe"
			},
			{
				"", "Hip",
				"Hip", "Right Arm 1", "Right Arm 2", "Right Hand", "Right Arm 2",
				"Hip", "Left Arm 1", "Left Arm 2", "Left Hand", "Left Arm 2",
				"Body",
				"Hip", "Left Leg 1", "Left Leg 2", "Left Foot",
				"Hip", "Right Leg 1", "Right Leg 2", "Right Foot"
			}
		},
		{"Tall_guy", math::Vec3i(70, 80, 70),
			{
				"Hip", "Body",
				"Shoulder",
				"Right Arm 1", "Right Arm 2", "Right Hand", "Right Finger", "Right Thumb",
				"Left Arm 1", "Left Arm 2", "Left Hand", "Left Finger", "Left Thumb",
				"Head",
				"Left Leg 1", "Left Leg 2", "Left Foot", "Left Toe",
				"Right Leg 1", "Right Leg 2", "Right Foot", "Right Toe",
			},
			{
				"", "Hip", "Body",
				"Hip", "Right Arm 1", "Right Arm 2", "Right Hand", "Right Arm 2",
				"Hip", "Left Arm 1", "Left Arm 2", "Left Hand", "Left Arm 2",
				"Shoulder",
				"Hip", "Left Leg 1", "Left Leg 2", "Left Foot",
				"Hip", "Right Leg 1", "Right Leg 2", "Right Foot",
				"Body"
			}
		},
		{"Red_Guy", math::Vec3i(40, 40, 40),
			{
				"Hip", "Body",
				"Right Arm 1", "Right Arm 2", "Right Hand", "Right Finger", "Right Thumb",
				"Left Arm 1", "Left Arm 2", "Left Hand", "Left Finger", "Left Thumb",
				"Head",
				"Body lower",
				"Left Leg 1", "Left Leg 2", "Left Foot", "Left Toe",
				"Right Leg 1", "Right Leg 2", "Right Foot", "Right Toe"
			},
			{
				"", "Body lower",
				"Hip", "Right Arm 1", "Right Arm 2", "Right Hand", "Right Arm 2",
				"Hip", "Left Arm 1", "Left Arm 2", "Left Hand", "Left Arm 2",
				"Body",
				"Hip",
				"Hip", "Left Leg 1", "Left Leg 2", "Left Foot",
				"Hip", "Right Leg 1", "Right Leg 2", "Right Foot",
			}
		},
		{"Biped_Boy", math::Vec3i(40, 40, 40),
			{
				"Torso", "Chest",
				"Arm Right 1", "Arm Right 2", "Hand Right", "Finger Right", "empty",
				"Arm Left 1", "Arm Left 2", "Hand Left", "Finger Left", "empty",
				"Head",
				"Foot Left 1", "Foot Left 2", "Feet Left", "empty",
				"Foot Right 1", "Foot Right 2", "Feet Right", "empty"
			},
			{
				"", "Torso",
				"Torso", "Arm Right 1", "Arm Right 2", "Hand Right", "",
				"Torso", "Arm Left 1", "Arm Left 2", "Hand Left", "",
				"Chest",
				"Torso", "Foot Left 1", "Foot Left 2", "",
				"Torso", "Foot Right 1", "Foot Right 2", ""
			}
		}
	};

	const SceneDefinition *sceneDefinition = nullptr;
	for (int i = 0; i < lengthof(sceneDefinitions); ++i) {
		const SceneDefinition &def = sceneDefinitions[i];
		if (sceneName == def.name) {
			sceneDefinition = &def;
			break;
		}
	}
	if (sceneDefinition == nullptr) {
		Log::error("Unknown scene type: %s", sceneName.c_str());
		return false;
	}
	math::Vec3i regionSize = sceneDefinition->size;
	Log::debug("scene size: %d %d %d", regionSize.x, regionSize.y, regionSize.z);

	core::Buffer<int> modelNodeIds;
	for (const auto &e : json.get("ModelSave")) {
		const int modelIdx = (int)modelNodeIds.size();
		regionSize = sceneDefinition->size;
		if (sceneName == "Quad_Simple") {
			if (modelIdx == 5 || modelIdx == 7) {
				regionSize = math::Vec3i(11);
			}
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		if (modelIdx >= (int)sceneDefinition->nodeNames.size()) {
			Log::error("No node name for model %d of scene %s", modelIdx, sceneName.c_str());
			node.setName(sceneName);
		} else {
			const core::String sceneNodeName = sceneDefinition->nodeNames[modelIdx];
			if (sceneNodeName.empty()) {
				Log::error("No node name for model %d of scene %s", modelIdx, sceneName.c_str());
				node.setName(sceneName);
			} else {
				node.setName(sceneDefinition->nodeNames[modelIdx]);
			}
		}
		int parent = 0;
		const core::String modelBase64 = e.str();
		io::MemoryReadStream inputStream(modelBase64.c_str(), modelBase64.size());
		io::Base64ReadStream base64Stream(inputStream);
		io::ZipReadStream readStream(base64Stream, -1, io::CompressionType::Gzip);
		const bool createVolumePalette = core::getVar(cfg::VoxelCreatePalette)->boolVal();
		palette::RGBABuffer colors;
		core::Buffer<color::RGBA> voxels(regionSize.x * regionSize.y * regionSize.z);
		for (color::RGBA &rgba : voxels) {
			AnimaToonVoxel v;
			if (readStream.readUInt8((uint8_t &)v.state) != 0 || readStream.readUInt8(v.val) != 0 ||
				readStream.readUInt32(v.rgba) != 0) {
				Log::error("Failed to read voxel data for model %d", modelIdx);
				return false;
			}
			rgba = color::RGBA(v.rgba);
			if (createVolumePalette && v.rgba != 0) {
				colors.insert(rgba);
			}
		}
		palette::Palette palette;
		if (createVolumePalette) {
			createPalette(colors, palette);
		}
		if (palette.colorCount() == 0) {
			palette = voxel::getPalette();
		}
		node.setPalette(palette);
		node.createVolume(voxel::Region::fromSize(regionSize));
		palette::PaletteLookup lookup(palette, palette::PaletteLookup::Precision::Exact);
		size_t voxelIndex = 0;
		for (int z = 0; z < regionSize.z; ++z) {
			for (int y = 0; y < regionSize.y; ++y) {
				for (int x = 0; x < regionSize.x; ++x) {
					const color::RGBA rgba = voxels[voxelIndex++];
					if (rgba.rgba != 0) {
						node.volume()->setVoxel(x, y, z, voxel::createVoxel(palette, lookup.findClosestIndex(rgba)));
					}
				}
			}
		}
		modelNodeIds.push_back(sceneGraph.emplace(core::move(node), parent));
	}

	for (int modelIdx = 0; modelIdx < (int)modelNodeIds.size(); ++modelIdx) {
		if (modelIdx < (int)sceneDefinition->parentNames.size()) {
			const core::String &parentName = sceneDefinition->parentNames[modelIdx];
			if (!parentName.empty()) {
				if (scenegraph::SceneGraphNode *parentNode = sceneGraph.findNodeByName(parentName)) {
					const int parent = parentNode->id();
					sceneGraph.changeParent(modelNodeIds[modelIdx], parent);
				} else {
					Log::error("Could not find parent node: %s", parentName.c_str());
				}
			}
		} else {
			Log::error("No parent name for model %d of scene %s", modelIdx, sceneName.c_str());
		}
	}

	math::Vec3f cameraPos(0.0f);
	math::Quatf cameraRot = math::Quatf::identity();
	math::Vec3f cameraTarget(0.0f);
	if (json.contains("MainCamPosition")) {
		json::Json mainCamPos = json.get("MainCamPosition");
		cameraPos = math::Vec3f(mainCamPos.floatVal("x", 0.0f), mainCamPos.floatVal("y", 0.0f),
							  mainCamPos.floatVal("z", 0.0f));
	}
	if (json.contains("MainCamRotation")) {
		json::Json mainCamRot = json.get("MainCamRotation");
		cameraRot = math::Quatf(mainCamRot.floatVal("x", 0.0f), mainCamRot.floatVal("y", 0.0f),
							  mainCamRot.floatVal("z", 0.0f), mainCamRot.floatVal("w", 0.0f));
	}
	if (json.contains("CamTargetPostion")) {
		json::Json camTargetPos = json.get("CamTargetPostion");
		cameraTarget = math::Vec3f(camTargetPos.floatVal("x", 0.0f), camTargetPos.floatVal("y", 0.0f),
								 camTargetPos.floatVal("z", 0.0f));
	}

	{
		const math::Mat4f &orientation = math::mat4_cast(cameraRot);
		const math::Mat4f &viewMatrix = math::translate(orientation, cameraPos);
		scenegraph::SceneGraphNodeCamera camNode;
		camNode.setName("Camera");
		scenegraph::SceneGraphTransform transform;
		transform.setWorldMatrix(viewMatrix);
		const scenegraph::KeyFrameIndex keyFrameIdx = 0;
		camNode.setTransform(keyFrameIdx, transform);
		camNode.setPerspective();
		sceneGraph.emplace(core::move(camNode), sceneGraph.root().id());
	}

#if 0
	core::DynamicArray<AnimaToonPosition> positions;
	for (const auto &savedPos : json.get("savedPositionsList")) {
		const std::string innerJson = savedPos.str();
		json::Json inner = json::Json::parse(innerJson);
		AnimaToonPosition pos;
		pos.isModified = inner.boolVal("isModified", false);
		pos.isLeftHandClosed = inner.boolVal("isLeftHandClosed", false);
		pos.isRightHandClosed = inner.boolVal("isRightHandClosed", false);
		for (const auto &meshPos : inner.get("meshPositions")) {
			const float x = meshPos.floatVal("x", 0.0f);
			const float y = meshPos.floatVal("y", 0.0f);
			const float z = meshPos.floatVal("z", 0.0f);
			pos.meshPositions.push_back(math::Vec3f(x, y, z));
		}
		for (const auto &meshRot : inner.get("meshRotations")) {
			const float x = meshRot.floatVal("x", 0.0f);
			const float y = meshRot.floatVal("y", 0.0f);
			const float z = meshRot.floatVal("z", 0.0f);
			const float w = meshRot.floatVal("w", 0.0f);
			pos.meshRotations.push_back(math::Quatf::wxyz(w, x, y, z));
		}
		for (const auto &ikPos : inner.get("IKPositions")) {
			const float x = ikPos.floatVal("x", 0.0f);
			const float y = ikPos.floatVal("y", 0.0f);
			const float z = ikPos.floatVal("z", 0.0f);
			pos.ikPositions.push_back(math::Vec3f(x, y, z));
		}
		for (const auto &ikRot : inner.get("IKRotations")) {
			const float x = ikRot.floatVal("x", 0.0f);
			const float y = ikRot.floatVal("y", 0.0f);
			const float z = ikRot.floatVal("z", 0.0f);
			const float w = ikRot.floatVal("w", 0.0f);
			pos.ikRotations.push_back(math::Quatf::wxyz(w, x, y, z));
		}
		for (const auto &ikMod : inner.get("IKModified")) {
			pos.ikModified.push_back(ikMod);
		}
		positions.push_back(pos);
	}

	// ImportModelSave contains imported external models as base64 encoded data
	// These would need to be loaded similarly to ModelSave but as separate imported nodes
	for (const auto &e : json.get("ImportModelSave")) {
		const core::String modelBase64 = e.str().c_str();
		if (modelBase64.empty()) {
			continue;
		}
		// TODO: VOXELFORMAT: implement ImportModelSave support when needed
		Log::debug("ImportModelSave entry found but not yet implemented");
	}

	for (size_t posIdx = 0; posIdx < positions.size(); ++posIdx) {
		const AnimaToonPosition &pos = positions[posIdx];
		const scenegraph::FrameIndex frameIdx = static_cast<scenegraph::FrameIndex>(posIdx);

		const size_t numNodes = core_min(modelNodeIds.size(), pos.meshPositions.size());
		for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
			const int nodeId = modelNodeIds[nodeIdx];
			if (!sceneGraph.hasNode(nodeId)) {
				Log::warn("Could not find node %d for animation frame %d", nodeId, (int)frameIdx);
				continue;
			}
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			scenegraph::KeyFrameIndex kfIdx = InvalidKeyFrame;
			if (node.hasKeyFrameForFrame(frameIdx, &kfIdx)) {
				Log::debug("Using existing keyframe at frame %d for node %d", (int)frameIdx, nodeId);
			} else {
				kfIdx = node.addKeyFrame(frameIdx);
				if (kfIdx == InvalidKeyFrame) {
					Log::warn("Failed to add keyframe at frame %d for node %d", (int)frameIdx, nodeId);
					continue;
				}
			}

			scenegraph::SceneGraphTransform trans;
			if (nodeIdx < pos.meshPositions.size()) {
				trans.setLocalTranslation(pos.meshPositions[nodeIdx]);
			}
			if (nodeIdx < pos.meshRotations.size()) {
				trans.setLocalOrientation(pos.meshRotations[nodeIdx]);
			}

			node.setTransform(kfIdx, trans);
		}
	}
#endif

	sceneGraph.updateTransforms();
	ctx.setProgress(1.0f);
	return true;
}

size_t AnimaToonFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
									palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const int64_t size = stream->size();
	core::String jsonStr(size, ' ');
	if (!stream->readString((int)jsonStr.size(), jsonStr.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	json::Json json = json::Json::parse(jsonStr);
	if (!json.contains("customColors") || !json.get("customColors").isArray()) {
		return 0u;
	}

	json::Json customColors = json.get("customColors");
	palette::ColorPalette colorPalette;
	for (const auto &e : customColors) {
		const uint8_t r = (uint8_t)(e.floatVal("r", 0.0f) * 255.0f);
		const uint8_t g = (uint8_t)(e.floatVal("g", 0.0f) * 255.0f);
		const uint8_t b = (uint8_t)(e.floatVal("b", 0.0f) * 255.0f);
		const uint8_t a = (uint8_t)(e.floatVal("a", 1.0f) * 255.0f);
		colorPalette.add(color::RGBA(r, g, b, a));
	}
	palette = palette::toPalette(colorPalette);
	return (size_t)palette.colorCount();
}

} // namespace voxelformat
