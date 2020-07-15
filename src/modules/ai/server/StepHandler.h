/**
 * @file
 */
#pragma once

#include "ai-shared/protocol/IProtocolHandler.h"

namespace ai {

class Server;

class StepHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit StepHandler(Server& server);

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
