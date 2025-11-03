/**
 * @file
 */

#include "SceneStateHandlerServer.h"
#include "voxedit-util/network/Server.h"

namespace voxedit {

SceneStateHandlerServer::SceneStateHandlerServer(Server *server) : _server(server) {
}

void SceneStateHandlerServer::execute(const ClientId &, SceneStateMessage *msg) {
	Log::info("Received scene state message with scene graph containing %i nodes", (int)msg->sceneGraph().size());
	_server->setSceneGraph(core::move(msg->sceneGraph()));
}

} // namespace voxedit
