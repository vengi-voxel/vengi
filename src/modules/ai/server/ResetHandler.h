#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class ResetHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	ResetHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
