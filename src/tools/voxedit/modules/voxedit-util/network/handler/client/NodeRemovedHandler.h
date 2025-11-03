/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRemovedMessage.h"

namespace voxedit {

class SceneManager;

class NodeRemovedHandler : public network::ProtocolTypeHandler<NodeRemovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRemovedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeRemovedMessage *message) override;
};

} // namespace voxedit
