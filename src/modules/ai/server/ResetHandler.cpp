/**
 * @file
 */

#include "ResetHandler.h"
#include "Server.h"

namespace ai {

ResetHandler::ResetHandler(Server& server) : _server(server) {
}

void ResetHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& /*message*/) {
	_server.reset();
}

}
