#pragma once

#include "IProtocolHandler.h"
#include "AIUpdateNodeMessage.h"

namespace ai {

class Server;

class UpdateNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	UpdateNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
