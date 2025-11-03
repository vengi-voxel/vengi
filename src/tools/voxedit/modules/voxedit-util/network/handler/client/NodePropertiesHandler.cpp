/**
 * @file
 */

#include "NodePropertiesHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodePropertiesHandler::NodePropertiesHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodePropertiesHandler::execute(const network::ClientId &, NodePropertiesMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received properties changed for unknown node UUID %s", uuidStr.c_str());
		return;
	}
	node->properties() = message->properties();
}

} // namespace voxedit
