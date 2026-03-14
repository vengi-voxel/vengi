/**
 * @file
 */

#include "AnimationAddTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

AnimationAddTool::AnimationAddTool() : Tool("voxedit_animation_add") {
	_tool.set("description",
		"Create a new animation in the scene. Use voxedit_animation_set to switch to it, then "
		"voxedit_node_add_keyframe to add keyframes for each node. Fetch the scene state to see existing "
		"animations and their keyframes.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("name");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("name", propTypeDescription("string", "Name of the new animation (e.g. 'walk', 'run', 'idle')"));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool AnimationAddTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	if (!args.contains("name") || !args.get("name").isString()) {
		return ctx.result(id, "Missing or invalid 'name' parameter", true);
	}
	const core::String name = args.get("name").str().c_str();
	if (name.empty()) {
		return ctx.result(id, "Animation name must not be empty", true);
	}
	if (ctx.sceneMgr->addAnimation(name)) {
		return ctx.result(id,
						  core::String::format("Animation '%s' created successfully. Use voxedit_animation_set "
											   "to switch to it and voxedit_node_add_keyframe to add keyframes.",
											   name.c_str()),
						  false);
	}
	return ctx.result(id, core::String::format("Failed to create animation '%s' - it may already exist", name.c_str()),
					  true);
}

} // namespace voxedit
