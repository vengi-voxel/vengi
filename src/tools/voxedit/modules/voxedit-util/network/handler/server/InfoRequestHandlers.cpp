/**
 * @file
 */

#include "InfoRequestHandlers.h"
#include "command/Command.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

void LuaScriptsRequestHandler::execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) {
	core::DynamicArray<LuaScriptInfo> scriptInfos;
	if (_luaApi != nullptr) {
		core::DynamicArray<voxelgenerator::LUAScript> scripts = _luaApi->listScripts();
		for (voxelgenerator::LUAScript &script : scripts) {
			if (script.cached) {
				continue;
			}
			_luaApi->reloadScriptParameters(script);
		}
		scriptInfos.reserve(scripts.size());
		for (const voxelgenerator::LUAScript &script : scripts) {
			if (!script.valid) {
				continue;
			}
			LuaScriptInfo info;
			info.filename = script.filename;
			info.description = script.desc;
			info.valid = script.valid;
			info.parameters.reserve(script.parameterDescription.size());
			for (const voxelgenerator::LUAParameterDescription &param : script.parameterDescription) {
				LuaParameterInfo paramInfo;
				paramInfo.name = param.name;
				paramInfo.description = param.description;
				paramInfo.defaultValue = param.defaultValue;
				paramInfo.enumValues = param.enumValues;
				paramInfo.minValue = param.minValue;
				paramInfo.maxValue = param.maxValue;
				paramInfo.type = (LuaParameterType)param.type;
				info.parameters.push_back(paramInfo);
			}
			scriptInfos.push_back(info);
		}
	}

	LuaScriptsListMessage response(scriptInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send lua scripts list to client %d", clientId);
	}
}

void LuaScriptCreateHandler::execute(const network::ClientId &clientId, LuaScriptCreateMessage *msg) {
	const core::VarPtr &password = core::Var::getSafe(cfg::VoxEditNetRconPassword);
	if (password->strVal() != msg->rconPassword()) {
		Log::warn("Received lua script create message with invalid rcon password from client id %d", clientId);
		return;
	}

	// Save the script to the filesystem
	const io::FilesystemPtr &fs = io::filesystem();
	const core::String path = core::String::format("scripts/%s.lua", msg->name().c_str());
	if (!fs->homeWrite(path, msg->content())) {
		Log::error("Failed to write lua script '%s'", path.c_str());
		return;
	}
	Log::info("Created lua script: %s", path.c_str());
}

void CVarsRequestHandler::execute(const network::ClientId &clientId, CVarsRequestMessage *msg) {
	core::DynamicArray<CVarInfo> cvarInfos;
	core::Var::visit([&cvarInfos](const core::VarPtr &var) {
		CVarInfo info;
		info.name = var->name();
		// Don't expose secret variable values
		if (var->getFlags() & core::CV_SECRET) {
			info.value = "***";
		} else {
			info.value = var->strVal();
		}
		const char *help = var->help();
		info.description = help != nullptr ? help : "";
		info.flags = var->getFlags();
		cvarInfos.push_back(info);
	});

	CVarsListMessage response(cvarInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send cvars list to client %d", clientId);
	}
}

void CommandsRequestHandler::execute(const network::ClientId &clientId, CommandsRequestMessage *msg) {
	core::DynamicArray<CommandInfo> commandInfos;
	command::Command::visit([&commandInfos](const command::Command &cmd) {
		CommandInfo info;
		info.name = cmd.name();
		info.description = cmd.help();
		commandInfos.push_back(info);
	});

	CommandsListMessage response(commandInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send commands list to client %d", clientId);
	}
}

} // namespace voxedit
