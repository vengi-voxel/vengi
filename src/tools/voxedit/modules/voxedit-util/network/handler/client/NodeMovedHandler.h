/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeMovedMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodeMovedHandler : public network::ProtocolTypeHandler<network::NodeMovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeMovedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodeMovedMessage *message) override;
};

} // namespace network
} // namespace voxedit
