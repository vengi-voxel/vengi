/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeAddedMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodeAddedHandler : public network::ProtocolTypeHandler<network::NodeAddedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeAddedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodeAddedMessage *message) override;
};

} // namespace network
} // namespace voxedit
