#include "ChangeHandler.h"
#include "Server.h"
#include "AIChangeMessage.h"

namespace ai {

void ChangeHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIChangeMessage& msg = static_cast<const AIChangeMessage&>(message);
	_server.setDebug(msg.getName());
}

}
