/**
 * @file
 */

#include "UpdateNodeHandler.h"
#include "Server.h"
#include "AIUpdateNodeMessage.h"

namespace ai {

UpdateNodeHandler::UpdateNodeHandler(Server& server) : _server(server) {
}

void UpdateNodeHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIUpdateNodeMessage& msg = static_cast<const AIUpdateNodeMessage&>(message);
	if (!_server.updateNode(msg.getCharacterId(), msg.getNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
		ai_log_error("Failed to update the node %u", msg.getNodeId());
	}
}

}
