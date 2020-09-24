/**
 * @file
 */

#include "DeleteNodeHandler.h"
#include "Server.h"
#include "core/Log.h"

namespace backend {

DeleteNodeHandler::DeleteNodeHandler(Server& server) : _server(server) {
}

void DeleteNodeHandler::executeWithRaw(void* attachment, const ai::DeleteNode* message, const uint8_t* rawData, size_t rawDataSize) {
	if (!_server.deleteNode(message->character_id(), message->node_id())) {
		Log::error("Failed to delete the node %u", message->node_id());
	}
}

}
