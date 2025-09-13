/**
 * @file
 */

#include "NodeMovedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

NodeMovedHandler::NodeMovedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeMovedHandler::execute(const network::ClientId &, network::NodeMovedMessage *message) {
	const core::String &nodeUUID = message->nodeUUID();
	const core::String &parentUUID = message->parentUUID();

	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		Log::warn("Received node moved for unknown node UUID %s", nodeUUID.c_str());
		return;
	}

	scenegraph::SceneGraphNode *newParent = nullptr;
	if (!parentUUID.empty()) {
		newParent = _sceneMgr->sceneGraph().findNodeByUUID(parentUUID);
		if (newParent == nullptr) {
			Log::warn("Received node moved for unknown parent UUID %s", parentUUID.c_str());
			return;
		}
	}

	Client &client = _sceneMgr->client();
	client.lockListener();

	const int newParentId = newParent ? newParent->id() : 0;
	_sceneMgr->nodeMove(node->id(), newParentId);

	client.unlockListener();
}

} // namespace network
} // namespace voxedit
