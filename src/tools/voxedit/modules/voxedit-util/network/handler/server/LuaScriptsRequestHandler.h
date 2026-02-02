/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

class ServerNetwork;

/**
 * @brief A client can issue an @c LuaScriptsRequestMessage to get the list of available lua scripts on the server side
 * @see @c LuaScriptCreateHandler
 */
class LuaScriptsRequestHandler : public network::ProtocolTypeHandler<LuaScriptsRequestMessage> {
private:
	ServerNetwork *_network;
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptsRequestHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi);
	void execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) override;
};

} // namespace voxedit
