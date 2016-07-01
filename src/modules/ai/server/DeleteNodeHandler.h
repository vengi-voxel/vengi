#pragma once

#include "IProtocolHandler.h"
#include "AIDeleteNodeMessage.h"
#include "Server.h"

namespace ai {

class Server;

class DeleteNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit DeleteNodeHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override {
		const AIDeleteNodeMessage& msg = static_cast<const AIDeleteNodeMessage&>(message);
		if (!_server.deleteNode(msg.getCharacterId(), msg.getNodeId())) {
			ai_log_error("Failed to delete the node %u", msg.getNodeId());
		}
	}
};

}
