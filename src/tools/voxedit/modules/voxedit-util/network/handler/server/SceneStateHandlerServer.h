/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {

class Server;

class SceneStateHandlerServer : public ProtocolTypeHandler<SceneStateMessage> {
private:
	Server *_server;

public:
	SceneStateHandlerServer(Server *server);
	void execute(const ClientId &, SceneStateMessage *msg) override;
};

} // namespace voxedit
