/**
 * @file
 */
#pragma once

#include "ai-shared/protocol/IProtocolHandler.h"

namespace backend {

class Server;

class DeleteNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit DeleteNodeHandler(Server& server);

	void execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) override;
};

}
