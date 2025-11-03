/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {

class Server;

class BroadcastHandler : public ProtocolHandler {
private:
	Server *_server;

public:
	BroadcastHandler(Server *server);
	void execute(const ClientId &, ProtocolMessage &msg) override;
};

} // namespace voxedit
