/**
 * @file
 */

#include "SelectHandler.h"
#include "Server.h"
#include "AISelectMessage.h"

namespace ai {

SelectHandler::SelectHandler(Server& server) : _server(server) {
}

void SelectHandler::execute(const ClientId& clientId, const IProtocolMessage& message) {
	const AISelectMessage& msg = static_cast<const AISelectMessage&>(message);
	_server.select(clientId, msg.getCharacterId());
}

}
