/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeAddedMessage.h"

namespace voxedit {

class SceneManager;

class NodeAddedHandler : public network::ProtocolTypeHandler<NodeAddedMessage> {
private:
	SceneManager *_sceneMgr;

public:
	NodeAddedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeAddedMessage *message) override;
};

} // namespace voxedit
