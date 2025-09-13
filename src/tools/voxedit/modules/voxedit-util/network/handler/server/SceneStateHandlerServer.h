/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {
namespace network {

class Server;

class SceneStateHandlerServer : public network::ProtocolTypeHandler<SceneStateMessage> {
private:
	Server *_server;

public:
	SceneStateHandlerServer(Server *server);
	void execute(const ClientId &, SceneStateMessage *msg) override;
};

} // namespace network
} // namespace voxedit
