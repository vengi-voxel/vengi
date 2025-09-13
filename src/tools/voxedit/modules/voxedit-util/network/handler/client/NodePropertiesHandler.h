/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePropertiesMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodePropertiesHandler : public network::ProtocolTypeHandler<network::NodePropertiesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePropertiesHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodePropertiesMessage *message) override;
};

} // namespace network
} // namespace voxedit
