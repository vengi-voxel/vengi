#include "ResetHandler.h"
#include "Server.h"

namespace ai {

void ResetHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& /*message*/) {
	_server.reset();
}

}
