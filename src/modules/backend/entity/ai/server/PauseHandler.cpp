/**
 * @file
 */

#include "PauseHandler.h"
#include "ai-shared/protocol/AIPauseMessage.h"
#include "Server.h"

namespace backend {

PauseHandler::PauseHandler(Server& server) : _server(server) {
}

void PauseHandler::execute(const ai::ClientId& clientId, const ai::IProtocolMessage& message) {
	const ai::AIPauseMessage& msg = static_cast<const ai::AIPauseMessage&>(message);
	_server.pause(clientId, msg.isPause());
}

}
