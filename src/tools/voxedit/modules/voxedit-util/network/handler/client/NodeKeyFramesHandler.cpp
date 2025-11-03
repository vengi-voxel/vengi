/**
 * @file
 */

#include "NodeKeyFramesHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeKeyFramesHandler::NodeKeyFramesHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeKeyFramesHandler::execute(const ClientId &, NodeKeyFramesMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received keyframes update for unknown node UUID %s", uuidStr.c_str());
		return;
	}

	const scenegraph::SceneGraphKeyFramesMap &keyFrames = message->keyFrames();

	Client &client = _sceneMgr->client();
	client.lockListener();
	const core::String activeAnimation = _sceneMgr->sceneGraph().activeAnimation();
	node->setAllKeyFrames(keyFrames, activeAnimation);
	_sceneMgr->sceneGraph().updateTransforms();
	client.unlockListener();
}

} // namespace voxedit
