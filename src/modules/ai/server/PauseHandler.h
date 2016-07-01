#pragma once

#include "IProtocolHandler.h"
#include "AIPauseMessage.h"
#include "Server.h"

namespace ai {

class Server;

class PauseHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit PauseHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override {
		const AIPauseMessage& msg = static_cast<const AIPauseMessage&>(message);
		_server.pause(clientId, msg.isPause());
	}
};

}
