/**
 * @file
 */

#include "ResetHandler.h"
#include "Server.h"

namespace backend {

ResetHandler::ResetHandler(Server& server) : _server(server) {
}

void ResetHandler::executeWithRaw(void*, const ai::Reset*, const uint8_t*, size_t) {
	_server.reset();
}

}
