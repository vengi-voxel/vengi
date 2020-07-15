/**
 * @file
 */
#pragma once

#include "ai-shared/protocol/IProtocolHandler.h"

namespace backend {

class Server;

class ChangeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit ChangeHandler(Server& server);

	void execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) override;
};

}
