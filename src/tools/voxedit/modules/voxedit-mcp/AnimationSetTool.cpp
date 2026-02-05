/**
 * @file
 */

#include "AnimationSetTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

AnimationSetTool::AnimationSetTool() : Tool("voxedit_animation_set") {
	_tool["description"] =
		"Switch the active animation by name. This must be called before adding keyframes to a specific animation. "
		"After switching, use voxedit_node_add_keyframe to create keyframes for nodes in this animation. "
		"Fetch the scene state to see the list of available animations.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"name"});
	inputSchema["properties"]["name"] =
		propTypeDescription("string", "Name of the animation to switch to (e.g. 'walk', 'run', 'idle', 'Default')");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool AnimationSetTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	if (!args.contains("name") || !args["name"].is_string()) {
		return ctx.result(id, "Missing or invalid 'name' parameter", true);
	}
	const core::String name = args["name"].get<std::string>().c_str();
	if (name.empty()) {
		return ctx.result(id, "Animation name must not be empty", true);
	}
	if (ctx.sceneMgr->setAnimation(name)) {
		return ctx.result(id,
						  core::String::format("Switched to animation '%s'. You can now add keyframes with "
											   "voxedit_node_add_keyframe.",
											   name.c_str()),
						  false);
	}
	return ctx.result(id,
					  core::String::format("Failed to switch to animation '%s' - it may not exist. "
										   "Fetch the scene state to see available animations.",
										   name.c_str()),
					  true);
}

} // namespace voxedit
