#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class StepHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	StepHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
