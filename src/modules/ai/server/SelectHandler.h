/**
 * @file
 */
#pragma once

#include "IProtocolHandler.h"
#include "AISelectMessage.h"
#include "Server.h"

namespace ai {

class SelectHandler: public ai::IProtocolHandler {
private:
	Server& _server;
public:
	explicit SelectHandler(Server& server) : _server(server) {
	}

	void execute(const ClientId& clientId, const IProtocolMessage& message) override {
		const AISelectMessage& msg = static_cast<const AISelectMessage&>(message);
		_server.select(clientId, msg.getCharacterId());
	}
};

}
