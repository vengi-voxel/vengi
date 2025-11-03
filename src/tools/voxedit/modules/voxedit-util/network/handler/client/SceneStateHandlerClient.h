/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {

class SceneManager;

class SceneStateHandlerClient : public ProtocolTypeHandler<SceneStateMessage> {
private:
	SceneManager *_sceneMgr;

public:
	SceneStateHandlerClient(SceneManager *sceneMgr);
	void execute(const ClientId &, SceneStateMessage *msg) override;
};

} // namespace voxedit
