/**
 * @file
 */

#include "BroadcastHandler.h"
#include "voxedit-util/network/Server.h"

namespace voxedit {
namespace network {

BroadcastHandler::BroadcastHandler(Server *server) : _server(server) {
}

void BroadcastHandler::execute(const ClientId &clientId, ProtocolMessage &msg) {
	Log::debug("Broadcasting message of type %d from client %d", msg.getId(), (int)clientId);
	msg.writeBack();
	_server->network().broadcast(msg, clientId);
}

} // namespace network
} // namespace voxedit
