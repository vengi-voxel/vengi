/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/CommandMessage.h"

namespace voxedit {
namespace network {

class Server;

class CommandHandlerServer : public network::ProtocolTypeHandler<CommandMessage> {
public:
	void execute(const ClientId &clientId, CommandMessage *msg) override {
		Log::info("Received command message: %s", msg->command().c_str());
		const core::VarPtr &password = core::Var::getSafe(cfg::VoxEditNetRconPassword);
		if (password->strVal() != msg->rconPassword()) {
			Log::warn("Received command message with invalid rcon password from client id %d", clientId);
			Log::error("password: '%s', expected: '%s'", msg->rconPassword().c_str(), password->strVal().c_str());
			return;
		}
		command::executeCommands(msg->command());
	}
};

} // namespace network
} // namespace voxedit
