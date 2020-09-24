/**
 * @file
 */

#include "UpdateNodeHandler.h"
#include "Server.h"
#include "core/Log.h"

namespace backend {

UpdateNodeHandler::UpdateNodeHandler(Server& server) : _server(server) {
}

void UpdateNodeHandler::executeWithRaw(void* attachment, const ai::UpdateNode* message, const uint8_t* rawData, size_t rawDataSize) {
	if (!_server.updateNode(message->character_id(), message->node_id(),
			message->name()->c_str(), message->type()->c_str(), message->condition()->c_str())) {
		Log::error("Failed to update the node %u", message->node_id());
	}
}

}
