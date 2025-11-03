/**
 * @file
 */

#include "SceneStateRequestHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

SceneStateRequestHandler::SceneStateRequestHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void SceneStateRequestHandler::execute(const ClientId &, SceneStateRequestMessage *message) {
	_sceneMgr->client().sendSceneState();
}

} // namespace voxedit
