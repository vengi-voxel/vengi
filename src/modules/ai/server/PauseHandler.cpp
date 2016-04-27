#include "PauseHandler.h"
#include "Server.h"

namespace ai {

void PauseHandler::execute(const ClientId& clientId, const IProtocolMessage& message) {
	const AIPauseMessage& msg = static_cast<const AIPauseMessage&>(message);
	_server.pause(clientId, msg.isPause());
}

}
