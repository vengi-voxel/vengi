/**
 * @file
 */

#include "NodePropertiesHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

NodePropertiesHandler::NodePropertiesHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodePropertiesHandler::execute(const network::ClientId &, network::NodePropertiesMessage *message) {
	const core::String &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		Log::warn("Received properties changed for unknown node UUID %s", uuid.c_str());
		return;
	}
	node->properties() = message->properties();
}

} // namespace network
} // namespace voxedit
