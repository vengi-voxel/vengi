/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeMovedMessage.h"

namespace voxedit {

class SceneManager;

class NodeMovedHandler : public network::ProtocolTypeHandler<NodeMovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeMovedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeMovedMessage *message) override;
};

} // namespace voxedit
