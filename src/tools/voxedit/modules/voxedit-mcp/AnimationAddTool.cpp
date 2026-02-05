/**
 * @file
 */

#include "AnimationAddTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

AnimationAddTool::AnimationAddTool() : Tool("voxedit_animation_add") {
	_tool["description"] =
		"Create a new animation in the scene. After creating, use voxedit_animation_set to switch to it, then "
		"use voxedit_node_add_keyframe to add keyframes for each node. Fetch the scene state to see existing "
		"animations and their keyframes.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"name"});
	inputSchema["properties"]["name"] =
		propTypeDescription("string", "Name of the new animation (e.g. 'walk', 'run', 'idle')");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool AnimationAddTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	if (!args.contains("name") || !args["name"].is_string()) {
		return ctx.result(id, "Missing or invalid 'name' parameter", true);
	}
	const core::String name = args["name"].get<std::string>().c_str();
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
