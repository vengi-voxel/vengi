/**
 * @file
 */

#include "ScriptApiTool.h"
#include "io/BufferedReadWriteStream.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

ScriptApiTool::ScriptApiTool() : Tool("voxedit_script_api") {
	_tool["description"] = "Lua API documentation as JSON. Describes all Lua functions and "
						   "parameters for writing scripts.";
	_tool["inputSchema"]["type"] = "object";
}

bool ScriptApiTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	io::BufferedReadWriteStream stream;
	voxelgenerator::LUAApi &luaApi = ctx.sceneMgr->luaApi();
	if (luaApi.apiJsonToStream(stream)) {
		core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
		return ctx.result(id, json, false);
	}
	return ctx.result(id, "Failed to generate Lua API documentation", true);
}

} // namespace voxedit
