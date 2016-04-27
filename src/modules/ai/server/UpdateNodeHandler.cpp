#include "UpdateNodeHandler.h"
#include "Server.h"

namespace ai {

void UpdateNodeHandler::execute(const ClientId&, const IProtocolMessage& message) {
	const AIUpdateNodeMessage& msg = static_cast<const AIUpdateNodeMessage&>(message);
	if (!_server.updateNode(msg.getCharacterId(), msg.getNodeId(), msg.getName(), msg.getType(), msg.getCondition()))
		std::cout << "Failed to update the node" << std::endl;
}

}
