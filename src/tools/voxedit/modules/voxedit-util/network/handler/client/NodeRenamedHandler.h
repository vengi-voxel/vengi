/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRenamedMessage.h"

namespace voxedit {

class SceneManager;

class NodeRenamedHandler : public network::ProtocolTypeHandler<NodeRenamedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRenamedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeRenamedMessage *message) override;
};

} // namespace voxedit
