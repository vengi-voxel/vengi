/**
 * @file
 */

#include "AddNodeHandler.h"
#include "ai-shared/protocol/AIAddNodeMessage.h"
#include "Server.h"
#include "backend/entity/ai/common/Log.h"

namespace backend {

AddNodeHandler::AddNodeHandler(Server& server) : _server(server) {
}

void AddNodeHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) {
	const ai::AIAddNodeMessage& msg = static_cast<const ai::AIAddNodeMessage&>(message);
	if (!_server.addNode(msg.getCharacterId(), msg.getParentNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
		ai_log_error("Failed to add the new node");
	}
}

}
