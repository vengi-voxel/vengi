#include "DeleteNodeHandler.h"
#include "Server.h"

namespace ai {

void DeleteNodeHandler::execute(const ClientId&, const IProtocolMessage& message) {
	const AIDeleteNodeMessage& msg = static_cast<const AIDeleteNodeMessage&>(message);
	if (!_server.deleteNode(msg.getCharacterId(), msg.getNodeId()))
		std::cout << "Failed to delete the node" << std::endl;
}

}
