/**
 * @file
 */

#include "SelectHandler.h"
#include "Server.h"
#include "ai-shared/protocol/AISelectMessage.h"

namespace backend {

SelectHandler::SelectHandler(Server& server) : _server(server) {
}

void SelectHandler::execute(const ai::ClientId& clientId, const ai::IProtocolMessage& message) {
	const ai::AISelectMessage& msg = static_cast<const ai::AISelectMessage&>(message);
	_server.select(clientId, msg.getCharacterId());
}

}
