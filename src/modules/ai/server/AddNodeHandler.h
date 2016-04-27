#pragma once

#include "IProtocolHandler.h"
#include "AIAddNodeMessage.h"

namespace ai {

class Server;

class AddNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	AddNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
