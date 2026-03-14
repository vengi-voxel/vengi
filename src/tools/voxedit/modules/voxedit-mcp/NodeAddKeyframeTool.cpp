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
	_tool.set("description",
		"Add a keyframe for a node at a frame index in the active animation. "
		"Use voxedit_animation_set to change the animation by name. "
		"After adding the keyframe, you can set the transform (translation, rotation, scale) "
		"for the node at that frame. The transform angles are in degrees (Euler angles). "
		"To create a walking animation, add keyframes at different frame indices with appropriate "
		"translations and rotations for leg, arm, and torso nodes. "
		"Fetch the scene state (with animations included) to see the current keyframes and node UUIDs.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("frameIdx");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", propUUID());
	inputSchema.get("properties").set("frameIdx",
		propTypeDescription("integer", "The frame index at which to create the keyframe (0-based). "
									   "Frame 0 is the first frame. Use different frame indices to create "
									   "animation poses at different points in time."));
	inputSchema.get("properties").get("frameIdx").set("minimum", 0);

	// Optional transform properties
	json::Json translationProp = json::Json::object();
	translationProp.set("type", "object");
	translationProp.set("description", "World-space translation of the node at this keyframe. "
									 "Y is up. Units are in voxels.");
	translationProp.get("properties").set("x", propTypeDescription("number", "X translation"));
	translationProp.get("properties").get("x").set("default", 0.0);
	translationProp.get("properties").set("y", propTypeDescription("number", "Y translation (up)"));
	translationProp.get("properties").get("y").set("default", 0.0);
	translationProp.get("properties").set("z", propTypeDescription("number", "Z translation"));
	translationProp.get("properties").get("z").set("default", 0.0);
	inputSchema.get("properties").set("translation", core::move(translationProp));

	json::Json rotationProp = json::Json::object();
	rotationProp.set("type", "object");
	rotationProp.set("description", "Euler rotation angles in degrees for the node at this keyframe. "
								  "Applied as XYZ rotation.");
	rotationProp.get("properties").set("x", propTypeDescription("number", "Rotation around X axis in degrees (pitch)"));
	rotationProp.get("properties").get("x").set("default", 0.0);
	rotationProp.get("properties").set("y", propTypeDescription("number", "Rotation around Y axis in degrees (yaw)"));
	rotationProp.get("properties").get("y").set("default", 0.0);
	rotationProp.get("properties").set("z", propTypeDescription("number", "Rotation around Z axis in degrees (roll)"));
	rotationProp.get("properties").get("z").set("default", 0.0);
	inputSchema.get("properties").set("rotation", core::move(rotationProp));

	json::Json scaleProp = json::Json::object();
	scaleProp.set("type", "object");
	scaleProp.set("description", "Scale of the node at this keyframe.");
	scaleProp.get("properties").set("x", propTypeDescription("number", "X scale"));
	scaleProp.get("properties").get("x").set("default", 1.0);
	scaleProp.get("properties").set("y", propTypeDescription("number", "Y scale"));
	scaleProp.get("properties").get("y").set("default", 1.0);
	scaleProp.get("properties").set("z", propTypeDescription("number", "Z scale"));
	scaleProp.get("properties").get("z").set("default", 1.0);
	inputSchema.get("properties").set("scale", core::move(scaleProp));

	json::Json interpolationProp = json::Json::object();
	interpolationProp.set("type", "string");
	interpolationProp.set("description", "The interpolation type between this keyframe and the next. "
									   "Controls how the transition is animated.");
	json::Json interpEnum = json::Json::array();
	interpEnum.push("Instant");
	interpEnum.push("Linear");
	interpEnum.push("QuadEaseIn");
	interpEnum.push("QuadEaseOut");
	interpEnum.push("QuadEaseInOut");
	interpEnum.push("CubicEaseIn");
	interpEnum.push("CubicEaseOut");
	interpEnum.push("CubicEaseInOut");
	interpEnum.push("CubicBezier");
	interpEnum.push("CatmullRom");
	interpolationProp.set("enum", interpEnum);
	interpolationProp.set("default", "Linear");
	inputSchema.get("properties").set("interpolation", core::move(interpolationProp));

	_tool.set("inputSchema", core::move(inputSchema));
}

static scenegraph::InterpolationType parseInterpolationType(const core::String &str) {
	for (int i = 0; i < (int)scenegraph::InterpolationType::Max; ++i) {
		if (str == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Linear;
}

bool NodeAddKeyframeTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("frameIdx") || !args.get("frameIdx").isNumberInteger()) {
		return ctx.result(id, "Missing or invalid 'frameIdx' parameter", true);
	}
	const scenegraph::FrameIndex frameIdx = args.get("frameIdx").intVal();
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
	if (args.contains("interpolation") && args.get("interpolation").isString()) {
		const core::String interpStr = args.get("interpolation").str().c_str();
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

		if (hasTranslation && args.get("translation").isObject()) {
			const auto &t = args.get("translation");
			translation.x = t.floatVal("x", 0.0f);
			translation.y = t.floatVal("y", 0.0f);
			translation.z = t.floatVal("z", 0.0f);
		}
		if (hasRotation && args.get("rotation").isObject()) {
			const auto &r = args.get("rotation");
			angles.x = r.floatVal("x", 0.0f);
			angles.y = r.floatVal("y", 0.0f);
			angles.z = r.floatVal("z", 0.0f);
		}
		if (hasScale && args.get("scale").isObject()) {
			const auto &s = args.get("scale");
			scale.x = s.floatVal("x", 1.0f);
			scale.y = s.floatVal("y", 1.0f);
			scale.z = s.floatVal("z", 1.0f);
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
