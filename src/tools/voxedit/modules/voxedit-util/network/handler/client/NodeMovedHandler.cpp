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
	const core::UUID &nodeUUID = message->nodeUUID();
	const core::UUID &parentUUID = message->parentUUID();

	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		const core::String &uuidStr = nodeUUID.str();
		Log::warn("Received node moved for unknown node UUID %s", uuidStr.c_str());
		return;
	}

	scenegraph::SceneGraphNode *newParent = nullptr;
	if (parentUUID.isValid()) {
		newParent = _sceneMgr->sceneGraph().findNodeByUUID(parentUUID);
		if (newParent == nullptr) {
			const core::String &uuidStr = parentUUID.str();
			Log::warn("Received node moved for unknown parent UUID %s", uuidStr.c_str());
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
