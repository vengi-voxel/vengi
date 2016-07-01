#pragma once

#include "IProtocolHandler.h"
#include "AIUpdateNodeMessage.h"
#include "Server.h"

namespace ai {

class Server;

class UpdateNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit UpdateNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override {
		const AIUpdateNodeMessage& msg = static_cast<const AIUpdateNodeMessage&>(message);
		if (!_server.updateNode(msg.getCharacterId(), msg.getNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
			ai_log_error("Failed to update the node %u", msg.getNodeId());
		}
	}
};

}
