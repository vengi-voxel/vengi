/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"

namespace voxedit {

class SceneManager;

class SceneStateRequestHandler : public ProtocolTypeHandler<SceneStateRequestMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	SceneStateRequestHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, SceneStateRequestMessage *message) override;
};

} // namespace voxedit
