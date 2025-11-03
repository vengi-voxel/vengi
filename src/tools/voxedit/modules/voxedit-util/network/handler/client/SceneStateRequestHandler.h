/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"

namespace voxedit {

class SceneManager;

class SceneStateRequestHandler : public network::ProtocolTypeHandler<SceneStateRequestMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	SceneStateRequestHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, SceneStateRequestMessage *message) override;
};

} // namespace voxedit
