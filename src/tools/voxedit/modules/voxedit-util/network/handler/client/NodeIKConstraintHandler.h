/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeIKConstraintMessage.h"

namespace voxedit {

class SceneManager;

class NodeIKConstraintHandler : public network::ProtocolTypeHandler<NodeIKConstraintMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeIKConstraintHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeIKConstraintMessage *message) override;
};

} // namespace voxedit
