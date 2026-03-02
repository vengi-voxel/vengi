/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/ClientListMessage.h"

namespace voxedit {

class SceneManager;

/**
 * @brief Client-side handler for connected client list updates from the server.
 */
class ClientListHandler : public network::ProtocolTypeHandler<ClientListMessage> {
private:
	SceneManager *_sceneMgr;

public:
	ClientListHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &clientId, ClientListMessage *msg) override;
};

} // namespace voxedit
