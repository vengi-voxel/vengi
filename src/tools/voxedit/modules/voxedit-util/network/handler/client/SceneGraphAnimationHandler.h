/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/SceneGraphAnimationMessage.h"

namespace voxedit {

class SceneManager;

class SceneGraphAnimationHandler : public network::ProtocolTypeHandler<SceneGraphAnimationMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	SceneGraphAnimationHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, SceneGraphAnimationMessage *message) override;
};

} // namespace voxedit
