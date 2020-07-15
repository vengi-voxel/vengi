/**
 * @file
 */

#include "ResetHandler.h"
#include "Server.h"

namespace backend {

ResetHandler::ResetHandler(Server& server) : _server(server) {
}

void ResetHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& /*message*/) {
	_server.reset();
}

}
