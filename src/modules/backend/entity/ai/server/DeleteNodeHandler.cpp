/**
 * @file
 */

#include "DeleteNodeHandler.h"
#include "ai-shared/protocol/AIDeleteNodeMessage.h"
#include "Server.h"
#include "core/Log.h"

namespace backend {

DeleteNodeHandler::DeleteNodeHandler(Server& server) : _server(server) {
}

void DeleteNodeHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) {
	const ai::AIDeleteNodeMessage& msg = static_cast<const ai::AIDeleteNodeMessage&>(message);
	if (!_server.deleteNode(msg.getCharacterId(), msg.getNodeId())) {
		Log::error("Failed to delete the node %u", msg.getNodeId());
	}
}

}
