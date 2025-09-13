/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRemovedMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodeRemovedHandler : public network::ProtocolTypeHandler<network::NodeRemovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRemovedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodeRemovedMessage *message) override;
};

} // namespace network
} // namespace voxedit
