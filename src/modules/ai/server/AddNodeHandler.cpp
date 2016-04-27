#include "AddNodeHandler.h"
#include "Server.h"

namespace ai {

void AddNodeHandler::execute(const ClientId&, const IProtocolMessage& message) {
	const AIAddNodeMessage& msg = static_cast<const AIAddNodeMessage&>(message);
	if (!_server.addNode(msg.getCharacterId(), msg.getParentNodeId(), msg.getName(), msg.getType(), msg.getCondition()))
		std::cout << "Failed to add the new node" << std::endl;
}

}
