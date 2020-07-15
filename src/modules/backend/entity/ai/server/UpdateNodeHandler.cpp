/**
 * @file
 */

#include "UpdateNodeHandler.h"
#include "Server.h"
#include "ai-shared/protocol/AIUpdateNodeMessage.h"
#include "core/Log.h"

namespace backend {

UpdateNodeHandler::UpdateNodeHandler(Server& server) : _server(server) {
}

void UpdateNodeHandler::execute(const ai::ClientId& /*clientId*/, const ai::IProtocolMessage& message) {
	const ai::AIUpdateNodeMessage& msg = static_cast<const ai::AIUpdateNodeMessage&>(message);
	if (!_server.updateNode(msg.getCharacterId(), msg.getNodeId(), msg.getName(), msg.getType(), msg.getCondition())) {
		Log::error("Failed to update the node %u", msg.getNodeId());
	}
}

}
