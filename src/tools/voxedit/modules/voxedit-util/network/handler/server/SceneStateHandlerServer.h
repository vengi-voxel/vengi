/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {

class Server;

class SceneStateHandlerServer : public network::ProtocolTypeHandler<SceneStateMessage> {
private:
	Server *_server;

public:
	SceneStateHandlerServer(Server *server);
	void execute(const network::ClientId &, SceneStateMessage *msg) override;
};

} // namespace voxedit
