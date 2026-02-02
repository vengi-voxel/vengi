/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/CVarsRequestMessage.h"

namespace voxedit {

class ServerNetwork;

/**
 * @see @c CommandsRequestHandler
 * @see @c LuaScriptsRequestHandler
 */
class CVarsRequestHandler : public network::ProtocolTypeHandler<CVarsRequestMessage> {
private:
	ServerNetwork *_network;

public:
	CVarsRequestHandler(ServerNetwork *network);
	void execute(const network::ClientId &clientId, CVarsRequestMessage *msg) override;
};

} // namespace voxedit
