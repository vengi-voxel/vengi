/**
 * @file
 */

#include "PauseHandler.h"
#include "Server.h"

namespace backend {

PauseHandler::PauseHandler(Server& server) : _server(server) {
}

void PauseHandler::executeWithRaw(void* attachment, const ai::Pause* message, const uint8_t* rawData, size_t rawDataSize) {
	_server.pause(message->pause());
}

}
