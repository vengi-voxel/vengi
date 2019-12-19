/**
 * @file
 */

#include "StepHandler.h"
#include "Server.h"
#include "AIStepMessage.h"

namespace ai {

StepHandler::StepHandler(Server& server) : _server(server) {
}

void StepHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIStepMessage& msg = static_cast<const AIStepMessage&>(message);
	_server.step(msg.getStepMillis());
}

}
