/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LuaScriptExecMessage.h"

namespace voxedit {

class SceneManager;

/**
 * @brief Handles synchronous lua script execution on the server side.
 * This runs the script to completion (including all coroutine yields) before returning.
 * @see LuaScriptExecMessage
 */
class LuaScriptExecHandler : public network::ProtocolTypeHandler<LuaScriptExecMessage> {
private:
	SceneManager *_sceneMgr = nullptr;

public:
	LuaScriptExecHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &clientId, LuaScriptExecMessage *msg) override;
};

} // namespace voxedit
