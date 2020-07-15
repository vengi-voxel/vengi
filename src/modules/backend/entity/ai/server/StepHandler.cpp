/**
 * @file
 */

#include "StepHandler.h"
#include "Server.h"
#include "ai-shared/protocol/AIStepMessage.h"

namespace backend {

StepHandler::StepHandler(Server& server) : _server(server) {
}

void StepHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) {
	const ai::AIStepMessage& msg = static_cast<const ai::AIStepMessage&>(message);
	_server.step(msg.getStepMillis());
}

}
