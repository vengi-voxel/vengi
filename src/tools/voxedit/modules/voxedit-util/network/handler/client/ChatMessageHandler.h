/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/ChatMessage.h"

namespace voxedit {

class SceneManager;

/**
 * @brief Client-side handler for incoming chat messages. Adds them to the Client's chat log.
 */
class ChatMessageHandler : public network::ProtocolTypeHandler<ChatMessage> {
private:
	SceneManager *_sceneMgr;

public:
	ChatMessageHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &clientId, ChatMessage *msg) override;
};

} // namespace voxedit
