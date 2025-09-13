/**
 * @file
 */

#include "SceneStateHandlerClient.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

SceneStateHandlerClient::SceneStateHandlerClient(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void SceneStateHandlerClient::execute(const ClientId &, SceneStateMessage *msg) {
	Log::info("Received scene state message with scene graph containing %i nodes", (int)msg->sceneGraph().size());

	Client &client = _sceneMgr->client();
	client.lockListener();
	_sceneMgr->loadSceneGraph(core::move(msg->sceneGraph()));
	client.unlockListener();
}

} // namespace network
} // namespace voxedit
