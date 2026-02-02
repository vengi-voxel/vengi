/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxedit-util/network/protocol/CVarsRequestMessage.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"

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
	LuaScriptsRequestHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi) : _network(network), _luaApi(luaApi) {
	}
	void execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) override;
};

/**
 * @brief Handler for LuaScriptCreateMessage - creates a new lua script
 */
class LuaScriptCreateHandler : public network::ProtocolTypeHandler<LuaScriptCreateMessage> {
private:
	ServerNetwork *_network;
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptCreateHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi) : _network(network), _luaApi(luaApi) {
	}
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
