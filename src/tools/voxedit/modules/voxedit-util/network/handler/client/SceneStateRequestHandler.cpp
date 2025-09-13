/**
 * @file
 */

#include "SceneStateRequestHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

SceneStateRequestHandler::SceneStateRequestHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void SceneStateRequestHandler::execute(const network::ClientId &, network::SceneStateRequestMessage *message) {
	_sceneMgr->client().sendSceneState();
}

} // namespace network
} // namespace voxedit
