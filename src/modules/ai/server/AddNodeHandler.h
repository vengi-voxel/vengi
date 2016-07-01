#pragma once

#include "IProtocolHandler.h"
#include "AIAddNodeMessage.h"

namespace ai {

class Server;

class AddNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit AddNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override {
		const AIAddNodeMessage& msg = static_cast<const AIAddNodeMessage&>(message);
		if (!_server.addNode(msg.getCharacterId(), msg.getParentNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
			ai_log_error("Failed to add the new node");
		}
	}
};

}
