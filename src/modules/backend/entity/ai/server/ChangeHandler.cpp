/**
 * @file
 */

#include "ChangeHandler.h"
#include "Server.h"

namespace backend {

ChangeHandler::ChangeHandler(Server& server) : _server(server) {
}

void ChangeHandler::executeWithRaw(void* attachment, const ai::ChangeZone* message, const uint8_t* rawData, size_t rawDataSize) {
	_server.setDebug(message->name()->c_str());
}

}
