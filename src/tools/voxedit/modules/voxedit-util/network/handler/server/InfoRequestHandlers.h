/**
 * @file
 */

#pragma once

#include "command/Command.h"
#include "core/Var.h"
#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxedit-util/network/protocol/CVarsRequestMessage.h"
#include "voxedit-util/network/protocol/CVarsListMessage.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"
#include "voxedit-util/network/protocol/CommandsListMessage.h"
#include "voxedit-util/Config.h"

namespace voxelgenerator {
class LUAApi;
}

namespace voxedit {

class Server;
class ServerNetwork;

/**
 * @brief Handler for LuaScriptsRequestMessage - returns list of available lua scripts
 */
class LuaScriptsRequestHandler : public network::ProtocolTypeHandler<LuaScriptsRequestMessage> {
private:
	ServerNetwork *_network;
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptsRequestHandler(ServerNetwork *network) : _network(network) {
	}
	void setLuaApi(voxelgenerator::LUAApi *luaApi) {
		_luaApi = luaApi;
	}
	void execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) override;
};

/**
 * @brief Handler for LuaScriptCreateMessage - creates a new lua script
 */
class LuaScriptCreateHandler : public network::ProtocolTypeHandler<LuaScriptCreateMessage> {
public:
	void execute(const network::ClientId &clientId, LuaScriptCreateMessage *msg) override;
};

/**
 * @brief Handler for CVarsRequestMessage - returns list of cvars
 */
class CVarsRequestHandler : public network::ProtocolTypeHandler<CVarsRequestMessage> {
private:
	ServerNetwork *_network;

public:
	CVarsRequestHandler(ServerNetwork *network) : _network(network) {
	}
	void execute(const network::ClientId &clientId, CVarsRequestMessage *msg) override;
};

/**
 * @brief Handler for CommandsRequestMessage - returns list of commands
 */
class CommandsRequestHandler : public network::ProtocolTypeHandler<CommandsRequestMessage> {
private:
	ServerNetwork *_network;

public:
	CommandsRequestHandler(ServerNetwork *network) : _network(network) {
	}
	void execute(const network::ClientId &clientId, CommandsRequestMessage *msg) override;
};

} // namespace voxedit
