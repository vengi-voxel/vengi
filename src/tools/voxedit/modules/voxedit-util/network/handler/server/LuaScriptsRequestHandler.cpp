/**
 * @file
 */

#include "LuaScriptsRequestHandler.h"
#include "core/Log.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"

namespace voxedit {

LuaScriptsRequestHandler::LuaScriptsRequestHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi)
	: _network(network), _luaApi(luaApi) {
}

void LuaScriptsRequestHandler::execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) {
	core::DynamicArray<LuaScriptInfo> scriptInfos;
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

	LuaScriptsListMessage response(scriptInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send lua scripts list to client %d", clientId);
	}
}

} // namespace voxedit
