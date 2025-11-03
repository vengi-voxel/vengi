/**
 * @file
 */

#include "NodeRenamedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRenamedHandler::NodeRenamedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeRenamedHandler::execute(const network::ClientId &, NodeRenamedMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received node renamed for unknown node UUID %s", uuidStr.c_str());
		return;
	}

	const core::String &newName = message->name();

	Client &client = _sceneMgr->client();
	client.lockListener();

	_sceneMgr->nodeRename(node->id(), newName);

	client.unlockListener();
}

} // namespace voxedit
