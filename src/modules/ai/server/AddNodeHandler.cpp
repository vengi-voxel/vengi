/**
 * @file
 */

#include "AddNodeHandler.h"
#include "ai-shared/protocol/AIAddNodeMessage.h"
#include "Server.h"
#include "ai-shared/common/Log.h"

namespace ai {

AddNodeHandler::AddNodeHandler(Server& server) : _server(server) {
}

void AddNodeHandler::execute(const ClientId& /*clientId*/, const IProtocolMessage& message) {
	const AIAddNodeMessage& msg = static_cast<const AIAddNodeMessage&>(message);
	if (!_server.addNode(msg.getCharacterId(), msg.getParentNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
		ai_log_error("Failed to add the new node");
	}
}

}
