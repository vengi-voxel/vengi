/**
 * @file
 */

#include "NodeRemovedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

NodeRemovedHandler::NodeRemovedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeRemovedHandler::execute(const network::ClientId &, network::NodeRemovedMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::debug("Received node removed for unknown node UUID %s - already removed", uuidStr.c_str());
		return;
	}

	Client &client = _sceneMgr->client();
	client.lockListener();

	_sceneMgr->nodeRemove(node->id(), true);

	client.unlockListener();
}

} // namespace network
} // namespace voxedit
