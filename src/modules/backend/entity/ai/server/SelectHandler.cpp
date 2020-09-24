/**
 * @file
 */

#include "SelectHandler.h"
#include "Server.h"

namespace backend {

SelectHandler::SelectHandler(Server& server) : _server(server) {
}

void SelectHandler::executeWithRaw(void* attachment, const ai::Select* message, const uint8_t* rawData, size_t rawDataSize) {
	_server.select(message->character_id());
}

}
