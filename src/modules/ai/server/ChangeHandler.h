#pragma once

#include "IProtocolHandler.h"
#include "Server.h"
#include "AIChangeMessage.h"

namespace ai {

class Server;

class ChangeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit ChangeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override{
		const AIChangeMessage& msg = static_cast<const AIChangeMessage&>(message);
		_server.setDebug(msg.getName());
	}
};

}
