/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"

namespace ai {

class Server;

class ChangeHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit ChangeHandler(Server& server);

	void execute(const ClientId& /*clientId*/, const IProtocolMessage& message) override;
};

}
