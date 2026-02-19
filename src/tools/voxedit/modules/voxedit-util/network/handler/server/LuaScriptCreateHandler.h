/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

class ServerNetwork;

/**
 * @brief A client can issue the @c LuaScriptCreateMessage to create a new lua script on the server side. This is checking the rcon password.
 * @see @c LuaScriptsRequestHandler
 */
class LuaScriptCreateHandler : public network::ProtocolTypeHandler<LuaScriptCreateMessage> {
private:
	voxelgenerator::LUAApi *_luaApi = nullptr;

public:
	LuaScriptCreateHandler(voxelgenerator::LUAApi *luaApi);
	void execute(const network::ClientId &clientId, LuaScriptCreateMessage *msg) override;
};

} // namespace voxedit
