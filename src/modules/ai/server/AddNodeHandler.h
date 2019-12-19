/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class AddNodeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit AddNodeHandler(Server& server);

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override;
};

}
