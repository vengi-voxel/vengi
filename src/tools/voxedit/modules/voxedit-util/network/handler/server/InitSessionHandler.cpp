/**
 * @file
 */

#include "InitSessionHandler.h"
#include "voxedit-util/network/Server.h"

namespace voxedit {
namespace network {

InitSessionHandler::InitSessionHandler(Server *server) : _server(server) {
}

void InitSessionHandler::execute(const ClientId &clientId, InitSessionMessage *msg) {
	if (!_server->initSession(clientId, msg->protocolVersion(), msg->applicationVersion(), msg->username(), msg->isLocalServer())) {
		_server->disconnect(clientId);
	}
}

} // namespace network
} // namespace voxedit
