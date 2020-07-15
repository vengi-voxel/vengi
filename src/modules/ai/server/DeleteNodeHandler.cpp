/**
 * @file
 */

#include "DeleteNodeHandler.h"
#include "ai-shared/protocol/AIDeleteNodeMessage.h"
#include "Server.h"
#include "ai-shared/common/Log.h"

namespace ai {

DeleteNodeHandler::DeleteNodeHandler(Server& server) : _server(server) {
}

void DeleteNodeHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIDeleteNodeMessage& msg = static_cast<const AIDeleteNodeMessage&>(message);
	if (!_server.deleteNode(msg.getCharacterId(), msg.getNodeId())) {
		ai_log_error("Failed to delete the node %u", msg.getNodeId());
	}
}

}
