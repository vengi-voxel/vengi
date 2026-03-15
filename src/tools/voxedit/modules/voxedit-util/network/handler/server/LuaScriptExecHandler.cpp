/**
 * @file
 */

#include "LuaScriptExecHandler.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

LuaScriptExecHandler::LuaScriptExecHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void LuaScriptExecHandler::execute(const network::ClientId &clientId, LuaScriptExecMessage *msg) {
	const core::VarPtr &password = core::getVar(cfg::VoxEditNetRconPassword);
	if (password->strVal() != msg->rconPassword()) {
		Log::warn("Received lua script exec message with invalid rcon password from client id %d", clientId);
		return;
	}

	const core::String &scriptName = msg->scriptName();
	const core::String &argsStr = msg->args();

	core::DynamicArray<core::String> args;
	if (!argsStr.empty()) {
		core::string::splitString(argsStr, args, " ");
	}

	voxelgenerator::LUAApi &luaApi = _sceneMgr->luaApi();
	const core::String &luaCode = luaApi.load(scriptName);
	if (luaCode.empty()) {
		Log::error("Failed to load script '%s'", scriptName.c_str());
		return;
	}

	if (!_sceneMgr->runScriptSync(luaCode, args)) {
		Log::error("Failed to execute script '%s'", scriptName.c_str());
	}
}

} // namespace voxedit
