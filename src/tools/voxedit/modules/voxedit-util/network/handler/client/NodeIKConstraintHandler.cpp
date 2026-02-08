/**
 * @file
 */

#include "NodeIKConstraintHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeIKConstraintHandler::NodeIKConstraintHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeIKConstraintHandler::execute(const network::ClientId &, NodeIKConstraintMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received IK constraint changed for unknown node UUID %s", uuidStr.c_str());
		return;
	}
	const core::Optional<scenegraph::IKConstraint> &ikConstraint = message->ikConstraint();
	if (ikConstraint.hasValue()) {
		node->setIkConstraint(*ikConstraint.value());
	} else {
		node->removeIkConstraint();
	}
}

} // namespace voxedit
