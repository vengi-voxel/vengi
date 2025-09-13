/**
 * @file
 */

#include "NodeRenamedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

NodeRenamedHandler::NodeRenamedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeRenamedHandler::execute(const network::ClientId &, network::NodeRenamedMessage *message) {
	const core::String &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		Log::warn("Received node renamed for unknown node UUID %s", uuid.c_str());
		return;
	}

	const core::String &newName = message->name();

	Client &client = _sceneMgr->client();
	client.lockListener();

	_sceneMgr->nodeRename(node->id(), newName);

	client.unlockListener();
}

} // namespace network
} // namespace voxedit
