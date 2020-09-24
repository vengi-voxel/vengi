/**
 * @file
 */

#include "StepHandler.h"
#include "Server.h"

namespace backend {

StepHandler::StepHandler(Server& server) : _server(server) {
}

void StepHandler::executeWithRaw(void* attachment, const ai::Step* message, const uint8_t* rawData, size_t rawDataSize) {
	_server.step(message->millis());
}

}
