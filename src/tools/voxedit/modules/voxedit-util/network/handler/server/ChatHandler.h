/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/ChatMessage.h"

namespace voxedit {

class Server;

/**
 * @brief Server-side handler for chat messages. Looks up the sender name from the
 * connected client and broadcasts the message to all clients.
 */
class ChatHandler : public network::ProtocolTypeHandler<ChatMessage> {
private:
	Server *_server;

public:
	ChatHandler(Server *server);
	void execute(const network::ClientId &clientId, ChatMessage *msg) override;
};

} // namespace voxedit
