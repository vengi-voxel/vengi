/**
 * @file
 */
#pragma once

#include "ai-shared/protocol/IProtocolHandler.h"

namespace ai {

class Server;

class ResetHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit ResetHandler(Server& server);

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& /*message*/) override;
};

}
