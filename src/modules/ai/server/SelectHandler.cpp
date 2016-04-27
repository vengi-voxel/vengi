#include "SelectHandler.h"
#include "Server.h"

namespace ai {

void SelectHandler::execute(const ClientId& clientId, const IProtocolMessage& message) {
	const AISelectMessage& msg = static_cast<const AISelectMessage&>(message);
	_server.select(clientId, msg.getCharacterId());
}

}
