/**
 * @file
 */

#include "ChangeHandler.h"
#include "Server.h"
#include "ai-shared/protocol/AIChangeMessage.h"

namespace backend {

ChangeHandler::ChangeHandler(Server& server) : _server(server) {
}

void ChangeHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) {
	const ai::AIChangeMessage& msg = static_cast<const ai::AIChangeMessage&>(message);
	_server.setDebug(msg.getName());
}

}
