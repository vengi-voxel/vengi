/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class PauseHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit PauseHandler(Server& server);

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
