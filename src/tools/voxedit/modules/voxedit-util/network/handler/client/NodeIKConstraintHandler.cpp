/**
 * @file
 */

#include "NodeIKConstraintHandler.h"
#include "core/Log.h"
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
		// Prefer the dedicated wire-format effector UUID when present.
		const core::UUID &effectorUUID = message->effectorUUID();
		if (effectorUUID.isValid()) {
			ik.effectorUUID = effectorUUID;
		}
		node->setIkConstraint(ik);
	} else {
		node->removeIkConstraint();
	}
}

} // namespace voxedit
