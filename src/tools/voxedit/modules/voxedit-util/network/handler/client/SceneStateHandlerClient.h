/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {

class SceneManager;

class SceneStateHandlerClient : public network::ProtocolTypeHandler<SceneStateMessage> {
private:
	SceneManager *_sceneMgr;

public:
	SceneStateHandlerClient(SceneManager *sceneMgr);
	void execute(const network::ClientId &, SceneStateMessage *msg) override;
};

} // namespace voxedit
