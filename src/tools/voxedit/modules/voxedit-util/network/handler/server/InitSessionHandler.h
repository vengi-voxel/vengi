/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/InitSessionMessage.h"

namespace voxedit {

class Server;

class InitSessionHandler : public network::ProtocolTypeHandler<InitSessionMessage> {
private:
	Server *_server;

public:
	InitSessionHandler(Server *server);
	void execute(const network::ClientId &, InitSessionMessage *msg) override;
};

} // namespace voxedit
