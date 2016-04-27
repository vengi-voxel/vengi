#pragma once

#include "IProtocolHandler.h"
#include "AISelectMessage.h"

namespace ai {

class Server;

class SelectHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	SelectHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
