/**
 * @file
 */

#include "NodeRemovedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRemovedHandler::NodeRemovedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeRemovedHandler::execute(const network::ClientId &, NodeRemovedMessage *message) {
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

} // namespace voxedit
