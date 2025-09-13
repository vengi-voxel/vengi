/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

class Server;

class BroadcastHandler : public network::ProtocolHandler {
private:
	Server *_server;

public:
	BroadcastHandler(Server *server);
	void execute(const ClientId &, ProtocolMessage &msg) override;
};

} // namespace network
} // namespace voxedit
