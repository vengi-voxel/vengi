/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class SceneStateRequestHandler : public network::ProtocolTypeHandler<network::SceneStateRequestMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	SceneStateRequestHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::SceneStateRequestMessage *message) override;
};

} // namespace network
} // namespace voxedit
