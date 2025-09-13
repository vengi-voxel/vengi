/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRenamedMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodeRenamedHandler : public network::ProtocolTypeHandler<network::NodeRenamedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRenamedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodeRenamedMessage *message) override;
};

} // namespace network
} // namespace voxedit
