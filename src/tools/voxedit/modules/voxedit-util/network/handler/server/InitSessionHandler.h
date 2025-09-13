/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/InitSessionMessage.h"

namespace voxedit {
namespace network {

class Server;

class InitSessionHandler : public network::ProtocolTypeHandler<InitSessionMessage> {
private:
	Server *_server;

public:
	InitSessionHandler(Server *server);
	void execute(const ClientId &, InitSessionMessage *msg) override;
};

} // namespace network
} // namespace voxedit
