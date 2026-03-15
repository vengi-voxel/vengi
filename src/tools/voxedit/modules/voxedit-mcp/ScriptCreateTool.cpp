/**
 * @file
 */

#include "ScriptCreateTool.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

ScriptCreateTool::ScriptCreateTool() : Tool("voxedit_script_create") {
	_tool.set("description",
		"Create (xs command to run it) Lua scripts. main function receives: node, region, color, [custom args]\n"
		"Api details via voxedit_script_api. Always add a description function to the script:"
		"function description() return 'My script description' end\n"
		"Try to use arguments to make the scripts re-usable. Custom argument handling: function arguments() return { "
		"{ name = 'padding', desc = 'padding between nodes', type = 'int', default = '2' } } end");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("name");
	_requiredArr.push("code");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("name", propTypeDescription("string", "Script name"));
	inputSchema.get("properties").set("code", propTypeDescription("string", "Lua code"));
	inputSchema.get("properties").set("args", propTypeDescription("string", "Script arguments defined by function arguments() in the lua code [custom args]"));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ScriptCreateTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::String name = args.strVal("name", "").c_str();
	const core::String code = args.strVal("code", "").c_str();
	const core::String scriptArgs = args.strVal("args", "").c_str();

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
