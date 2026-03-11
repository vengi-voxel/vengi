/**
 * @file
 */

#include "ClientListHandler.h"
#include "core/Log.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

ClientListHandler::ClientListHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void ClientListHandler::execute(const network::ClientId &, ClientListMessage *msg) {
	Log::debug("Received client list update with %d clients", (int)msg->clients().size());
	_sceneMgr->client().updateConnectedClients(msg->clients());
}

} // namespace voxedit
