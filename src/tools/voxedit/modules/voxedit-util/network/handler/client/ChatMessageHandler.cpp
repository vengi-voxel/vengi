/**
 * @file
 */

#include "ChatMessageHandler.h"
#include "core/Log.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

ChatMessageHandler::ChatMessageHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void ChatMessageHandler::execute(const network::ClientId &, ChatMessage *msg) {
	Log::info("[Chat] %s: %s", msg->sender().c_str(), msg->message().c_str());
	_sceneMgr->client().addChatMessage(msg->sender(), msg->message(), msg->isSystem());
}

} // namespace voxedit
