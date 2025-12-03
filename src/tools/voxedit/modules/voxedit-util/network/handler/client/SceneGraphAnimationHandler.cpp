/**
 * @file
 */

#include "SceneGraphAnimationHandler.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/Client.h"

namespace voxedit {

SceneGraphAnimationHandler::SceneGraphAnimationHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void SceneGraphAnimationHandler::execute(const network::ClientId &, SceneGraphAnimationMessage *message) {
	Client &client = _sceneMgr->client();
	client.lockListener();

	_sceneMgr->sceneGraph().setAnimations(message->animations());

	client.unlockListener();
}

} // namespace voxedit
