#pragma once

#include "IProtocolHandler.h"
#include "Server.h"

namespace ai {

class ResetHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit ResetHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& /*message*/) override {
		_server.reset();
	}
};

}
