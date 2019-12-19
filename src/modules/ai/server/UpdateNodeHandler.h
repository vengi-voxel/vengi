/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class UpdateNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit UpdateNodeHandler(Server& server);

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
