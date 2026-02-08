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
		scenegraph::IKConstraint ik = *ikConstraint.value();
		// Resolve the effector UUID back to a local node ID
		const core::UUID &effectorUUID = message->effectorUUID();
		if (effectorUUID.isValid()) {
			const scenegraph::SceneGraphNode *effectorNode = _sceneMgr->sceneGraph().findNodeByUUID(effectorUUID);
			if (effectorNode != nullptr) {
				ik.effectorNodeId = effectorNode->id();
			} else {
				Log::warn("Effector node UUID %s not found", effectorUUID.str().c_str());
				ik.effectorNodeId = InvalidNodeId;
			}
		} else {
			ik.effectorNodeId = InvalidNodeId;
		}
		node->setIkConstraint(ik);
	} else {
		node->removeIkConstraint();
	}
}

} // namespace voxedit
