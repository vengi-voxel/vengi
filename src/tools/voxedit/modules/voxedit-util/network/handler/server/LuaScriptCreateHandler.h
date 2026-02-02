/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

class ServerNetwork;

class LuaScriptCreateHandler : public network::ProtocolTypeHandler<LuaScriptCreateMessage> {
private:
	ServerNetwork *_network;
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptCreateHandler(ServerNetwork *network, voxelgenerator::LUAApi *luaApi);
	void execute(const network::ClientId &clientId, LuaScriptCreateMessage *msg) override;
};

} // namespace voxedit
