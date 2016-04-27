#pragma once

#include "IProtocolHandler.h"
#include "AIPauseMessage.h"

namespace ai {

class Server;

class PauseHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	PauseHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
