/**
 * @file
 */

#include "ScriptCreateTool.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

ScriptCreateTool::ScriptCreateTool() : Tool("voxedit_script_create") {
	_tool["description"] =
		"Create a new Lua script. Script main function receives: node, region, color, [custom args]\n"
		"Get api details with voxedit_script_api. Always add a description function to your script:"
		"function description() return 'My script description' end\n"
		"Try to use arguments to make your scripts re-usable. Custom argument handling: function arguments() return { "
		"{ name = 'padding', desc = 'padding between nodes', type = 'int', default = '2' } } end";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"name", "code"});
	inputSchema["properties"]["name"] = propTypeDescription("string", "Script name");
	inputSchema["properties"]["code"] = propTypeDescription("string", "Lua script code");
	inputSchema["properties"]["args"] =
		propTypeDescription("string", "Script arguments defined by function arguments() in the lua code [custom args]");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool ScriptCreateTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::String name = args.value("name", "").c_str();
	const core::String code = args.value("code", "").c_str();
	const core::String scriptArgs = args.value("args", "").c_str();

	voxelgenerator::LUAApi &luaApi = ctx.sceneMgr->luaApi();
	voxelgenerator::LUAScript script;
	if (!luaApi.reloadScriptParameters(script, code)) {
		return ctx.result(id, core::String::format("Failed to create script: %s", luaApi.error().c_str()), true);
	}
	if (!script.valid) {
		return ctx.result(id, "Failed to create script: detected as invalid", true);
	}
	if (script.desc.empty()) {
		return ctx.result(id, "Failed to create script: missing description function", true);
	}

	voxedit::LuaScriptCreateMessage msg(name, code, rconPassword());
	if (!sendMessage(ctx, msg)) {
		return ctx.result(id, "Failed to create script", true);
	}
	voxedit::LuaScriptsRequestMessage requestMsg;
	sendMessage(ctx, requestMsg);

	return ctx.result(id, "Script created successfully", false);
}

} // namespace voxedit
