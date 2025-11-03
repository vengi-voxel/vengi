/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePropertiesMessage.h"

namespace voxedit {

class SceneManager;

class NodePropertiesHandler : public network::ProtocolTypeHandler<NodePropertiesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePropertiesHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodePropertiesMessage *message) override;
};

} // namespace voxedit
