#pragma once

#include "IProtocolHandler.h"
#include "AIDeleteNodeMessage.h"

namespace ai {

class Server;

class DeleteNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	DeleteNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
