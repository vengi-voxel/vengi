/**
 * @file
 */

#include "CommandsRequestHandler.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxedit-util/network/protocol/CommandsListMessage.h"

namespace voxedit {

CommandsRequestHandler::CommandsRequestHandler(ServerNetwork *network) : _network(network) {
}

void CommandsRequestHandler::execute(const network::ClientId &clientId, CommandsRequestMessage *msg) {
	core::DynamicArray<CommandInfo> commandInfos;
	command::Command::visit([&commandInfos](const command::Command &cmd) {
		CommandInfo info;
		info.name = cmd.name();
		info.description = cmd.help();
		commandInfos.push_back(info);
	});

	CommandsListMessage response(commandInfos);
	if (!_network->sendToClient(clientId, response)) {
		Log::error("Failed to send commands list to client %d", clientId);
	}
}

} // namespace voxedit
