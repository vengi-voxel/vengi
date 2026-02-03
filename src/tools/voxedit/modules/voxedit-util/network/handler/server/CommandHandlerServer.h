/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/CommandMessage.h"

namespace voxedit {

class Server;

/**
 * @brief A client can issue the @c CommandMessage to execute commands on the server side. This is checking the rcon password.
 */
class CommandHandlerServer : public network::ProtocolTypeHandler<CommandMessage> {
public:
	void execute(const network::ClientId &clientId, CommandMessage *msg) override {
		Log::info("Received command message: %s", msg->command().c_str());
		const core::VarPtr &password = core::Var::getSafe(cfg::VoxEditNetRconPassword);
		if (password->strVal() != msg->rconPassword()) {
			Log::warn("Received command message with invalid rcon password from client id %d", clientId);
			return;
		}
		command::executeCommands(msg->command());
	}
};

} // namespace voxedit
