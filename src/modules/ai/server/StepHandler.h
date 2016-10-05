/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"
#include "Server.h"
#include "AIStepMessage.h"

namespace ai {

class Server;

class StepHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit StepHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override {
		const AIStepMessage& msg = static_cast<const AIStepMessage&>(message);
		_server.step(msg.getStepMillis());
	}
};

}
