#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class ChangeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	ChangeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
