/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"

namespace voxedit {

class ServerNetwork;

/**
 * @see @c CVarsRequestHandler
 * @see @c LuaScriptsRequestHandler
 */
class CommandsRequestHandler : public network::ProtocolTypeHandler<CommandsRequestMessage> {
private:
	ServerNetwork *_network;

public:
	CommandsRequestHandler(ServerNetwork *network);
	void execute(const network::ClientId &clientId, CommandsRequestMessage *msg) override;
};

} // namespace voxedit
