/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

class ServerNetwork;

class LuaScriptsRequestHandler : public network::ProtocolTypeHandler<LuaScriptsRequestMessage> {
private:
	ServerNetwork *_network;
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptsRequestHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi);
	void execute(const network::ClientId &clientId, LuaScriptsRequestMessage *msg) override;
};

} // namespace voxedit
