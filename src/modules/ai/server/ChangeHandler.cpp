/**
 * @file
 */

#include "ChangeHandler.h"
#include "Server.h"
#include "ai-shared/protocol/AIChangeMessage.h"

namespace ai {

ChangeHandler::ChangeHandler(Server& server) : _server(server) {
}

void ChangeHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIChangeMessage& msg = static_cast<const AIChangeMessage&>(message);
	_server.setDebug(msg.getName());
}

}
