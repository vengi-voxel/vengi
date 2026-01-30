/**
 * @file
 */

#include "InitSessionHandler.h"
#include "voxedit-util/network/Server.h"

namespace voxedit {

InitSessionHandler::InitSessionHandler(Server *server) : _server(server) {
}

void InitSessionHandler::execute(const network::ClientId &clientId, InitSessionMessage *msg) {
	if (!_server->initSession(clientId, msg->protocolVersion(), msg->applicationVersion(), msg->username(), msg->password(), msg->isLocalServer())) {
		// Mark for disconnection - don't disconnect directly as it invalidates iterators
		_server->markForDisconnect(clientId);
	}
}

} // namespace voxedit
