/**
 * @file
 */

#include "NodeAddKeyframeTool.h"
#include "core/Log.h"
#include "core/UUID.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeAddKeyframeTool::NodeAddKeyframeTool() : Tool("voxedit_node_add_keyframe") {
	_tool["description"] =
		"Add a keyframe for a node at a specific frame index in the currently active animation. "
		"Use voxedit_animation_set first to switch to the desired animation before adding keyframes. "
		"After adding the keyframe, you can optionally set the transform (translation, rotation, scale) "
		"for the node at that frame. The transform angles are in degrees (Euler angles). "
		"To create a walking animation, add keyframes at different frame indices with appropriate "
		"translations and rotations for leg, arm, and torso nodes. "
		"Fetch the scene state (with animations included) to see the current keyframes and node UUIDs.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "frameIdx"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	inputSchema["properties"]["frameIdx"] =
		propTypeDescription("integer", "The frame index at which to create the keyframe (0-based). "
									   "Frame 0 is the first frame. Use different frame indices to create "
									   "animation poses at different points in time.");
	inputSchema["properties"]["frameIdx"]["minimum"] = 0;

	// Optional transform properties
	nlohmann::json translationProp;
	translationProp["type"] = "object";
	translationProp["description"] = "World-space translation of the node at this keyframe. "
									 "Y is up. Units are in voxels.";
	translationProp["properties"]["x"] = propTypeDescription("number", "X translation");
	translationProp["properties"]["x"]["default"] = 0.0;
	translationProp["properties"]["y"] = propTypeDescription("number", "Y translation (up)");
	translationProp["properties"]["y"]["default"] = 0.0;
	translationProp["properties"]["z"] = propTypeDescription("number", "Z translation");
	translationProp["properties"]["z"]["default"] = 0.0;
	inputSchema["properties"]["translation"] = core::move(translationProp);

	nlohmann::json rotationProp;
	rotationProp["type"] = "object";
	rotationProp["description"] = "Euler rotation angles in degrees for the node at this keyframe. "
								  "Applied as XYZ rotation.";
	rotationProp["properties"]["x"] = propTypeDescription("number", "Rotation around X axis in degrees (pitch)");
	rotationProp["properties"]["x"]["default"] = 0.0;
	rotationProp["properties"]["y"] = propTypeDescription("number", "Rotation around Y axis in degrees (yaw)");
	rotationProp["properties"]["y"]["default"] = 0.0;
	rotationProp["properties"]["z"] = propTypeDescription("number", "Rotation around Z axis in degrees (roll)");
	rotationProp["properties"]["z"]["default"] = 0.0;
	inputSchema["properties"]["rotation"] = core::move(rotationProp);

	nlohmann::json scaleProp;
	scaleProp["type"] = "object";
	scaleProp["description"] = "Scale of the node at this keyframe.";
	scaleProp["properties"]["x"] = propTypeDescription("number", "X scale");
	scaleProp["properties"]["x"]["default"] = 1.0;
	scaleProp["properties"]["y"] = propTypeDescription("number", "Y scale");
	scaleProp["properties"]["y"]["default"] = 1.0;
	scaleProp["properties"]["z"] = propTypeDescription("number", "Z scale");
	scaleProp["properties"]["z"]["default"] = 1.0;
	inputSchema["properties"]["scale"] = core::move(scaleProp);

	nlohmann::json interpolationProp;
	interpolationProp["type"] = "string";
	interpolationProp["description"] = "The interpolation type between this keyframe and the next. "
									   "Controls how the transition is animated.";
	interpolationProp["enum"] =
		nlohmann::json::array({"Instant", "Linear", "QuadEaseIn", "QuadEaseOut", "QuadEaseInOut", "CubicEaseIn",
							   "CubicEaseOut", "CubicEaseInOut", "CubicBezier", "CatmullRom"});
	interpolationProp["default"] = "Linear";
	inputSchema["properties"]["interpolation"] = core::move(interpolationProp);

	_tool["inputSchema"] = core::move(inputSchema);
}

static scenegraph::InterpolationType parseInterpolationType(const core::String &str) {
	for (int i = 0; i < (int)scenegraph::InterpolationType::Max; ++i) {
		if (str == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Linear;
}

bool NodeAddKeyframeTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("frameIdx") || !args["frameIdx"].is_number_integer()) {
		return ctx.result(id, "Missing or invalid 'frameIdx' parameter", true);
	}
	const scenegraph::FrameIndex frameIdx = args["frameIdx"].get<int>();
	if (frameIdx < 0) {
		return ctx.result(id, "frameIdx must be >= 0", true);
	}

	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph - fetch the scene state first", true);
	}
	const int nodeId = node->id();

	if (!ctx.sceneMgr->nodeAddKeyFrame(nodeId, frameIdx)) {
		return ctx.result(id,
						  core::String::format("Failed to add keyframe at frame %i for node %s. "
											   "A keyframe may already exist at that frame.",
											   (int)frameIdx, nodeUUID.str().c_str()),
						  true);
	}

	// After adding the keyframe, look up the KeyFrameIndex to apply transform and interpolation
	// Re-fetch the node pointer since the scene graph may have changed
	node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node disappeared after adding keyframe", true);
	}

	const scenegraph::KeyFrameIndex keyFrameIdx = node->keyFrameForFrame(frameIdx);
	if (keyFrameIdx == InvalidKeyFrame) {
		return ctx.result(id, "Keyframe was added but could not be found afterwards", true);
	}

	// Apply interpolation if specified
	if (args.contains("interpolation") && args["interpolation"].is_string()) {
		const core::String interpStr = args["interpolation"].get<std::string>().c_str();
		const scenegraph::InterpolationType interpType = parseInterpolationType(interpStr);
		ctx.sceneMgr->nodeUpdateKeyFrameInterpolation(nodeId, keyFrameIdx, interpType);
	}

	// Apply transform if any transform properties are specified
	const bool hasTranslation = args.contains("translation");
	const bool hasRotation = args.contains("rotation");
	const bool hasScale = args.contains("scale");

	if (hasTranslation || hasRotation || hasScale) {
		glm::vec3 translation(0.0f);
		glm::vec3 angles(0.0f);
		glm::vec3 scale(1.0f);

		if (hasTranslation && args["translation"].is_object()) {
			const auto &t = args["translation"];
			translation.x = t.value("x", 0.0f);
			translation.y = t.value("y", 0.0f);
			translation.z = t.value("z", 0.0f);
		}
		if (hasRotation && args["rotation"].is_object()) {
			const auto &r = args["rotation"];
			angles.x = r.value("x", 0.0f);
			angles.y = r.value("y", 0.0f);
			angles.z = r.value("z", 0.0f);
		}
		if (hasScale && args["scale"].is_object()) {
			const auto &s = args["scale"];
			scale.x = s.value("x", 1.0f);
			scale.y = s.value("y", 1.0f);
			scale.z = s.value("z", 1.0f);
		}

		// Use local=false for world-space transforms
		ctx.sceneMgr->nodeUpdateTransform(nodeId, angles, scale, translation, keyFrameIdx, false);
	}

	return ctx.result(id,
					  core::String::format("Added keyframe at frame %i for node %s with keyframe index %i. "
										   "Fetch the scene state to see the updated animation.",
										   (int)frameIdx, nodeUUID.str().c_str(), (int)keyFrameIdx),
					  false);
}

} // namespace voxedit
