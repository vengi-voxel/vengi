/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeMovedMessage.h"

namespace voxedit {

class SceneManager;

class NodeMovedHandler : public ProtocolTypeHandler<NodeMovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeMovedHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodeMovedMessage *message) override;
};

} // namespace voxedit
