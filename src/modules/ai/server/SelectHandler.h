/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class SelectHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit SelectHandler(Server& server);

	void execute(const ClientId& clientId, const IProtocolMessage& message) override;
};

}
