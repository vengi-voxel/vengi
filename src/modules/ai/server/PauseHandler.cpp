/**
 * @file
 */

#include "PauseHandler.h"
#include "AIPauseMessage.h"
#include "Server.h"

namespace ai {

PauseHandler::PauseHandler(Server& server) : _server(server) {
}

void PauseHandler::execute(const ClientId& clientId, const IProtocolMessage& message) {
	const AIPauseMessage& msg = static_cast<const AIPauseMessage&>(message);
	_server.pause(clientId, msg.isPause());
}

}
