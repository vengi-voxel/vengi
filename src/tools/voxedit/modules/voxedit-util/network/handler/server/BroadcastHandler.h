/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "network/ProtocolMessage.h"

namespace voxedit {

class Server;

class BroadcastHandler : public network::ProtocolHandler {
private:
	Server *_server;

public:
	BroadcastHandler(Server *server);
	void execute(const network::ClientId &, network::ProtocolMessage &msg) override;
};

} // namespace voxedit
