#include "LuaScriptCreateHandler.h"
#include "app/App.h"
#include "commonlua/LUA.h"
#include "core/Log.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "voxedit-util/Config.h"

namespace voxedit {

LuaScriptCreateHandler::LuaScriptCreateHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi)
	: _network(network), _luaApi(luaApi) {
}

void LuaScriptCreateHandler::execute(const network::ClientId &clientId, LuaScriptCreateMessage *msg) {
	const core::VarPtr &password = core::Var::getSafe(cfg::VoxEditNetRconPassword);
	if (password->strVal() != msg->rconPassword()) {
		Log::warn("Received lua script create message with invalid rcon password from client id %d", clientId);
		return;
	}

	lua::LUA lua;
	if (!_luaApi->prepare(lua, msg->content())) {
		Log::error("Invalid lua script content from client %d: %s", clientId, lua.error().c_str());
		return;
	}

	const io::FilesystemPtr &fs = io::filesystem();
	const core::String path = core::String::format("scripts/%s.lua", msg->name().c_str());
	if (!fs->homeWrite(path, msg->content())) {
		Log::error("Failed to write lua script '%s'", path.c_str());
		return;
	}
	Log::info("Created lua script: %s", path.c_str());
}

} // namespace voxedit
