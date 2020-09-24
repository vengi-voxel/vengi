/**
 * @file
 */

#include "AddNodeHandler.h"
#include "Server.h"
#include "core/Log.h"

namespace backend {

AddNodeHandler::AddNodeHandler(Server& server) : _server(server) {
}

void AddNodeHandler::executeWithRaw(void* attachment, const ai::AddNode* message, const uint8_t* rawData, size_t rawDataSize) {
	if (!_server.addNode(message->character_id(), message->parent_node_id(),
			message->name()->c_str(), message->type()->c_str(), message->condition()->c_str())) {
		Log::error("Failed to add the new node");
	}
}

}
