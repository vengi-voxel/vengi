/**
 * @file
 */

#include "ChatHandler.h"
#include "voxedit-util/network/Server.h"

namespace voxedit {

ChatHandler::ChatHandler(Server *server) : _server(server) {
}

void ChatHandler::execute(const network::ClientId &clientId, ChatMessage *msg) {
	const core::String senderName = _server->disambiguatedName(clientId);
	Log::info("Chat from %s: %s", senderName.c_str(), msg->message().c_str());
	ChatMessage broadcastMsg(senderName, msg->message(), false);
	_server->network().broadcast(broadcastMsg);
}

} // namespace voxedit
