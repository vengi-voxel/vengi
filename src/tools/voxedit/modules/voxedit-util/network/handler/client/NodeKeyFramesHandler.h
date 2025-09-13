/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeKeyFramesMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodeKeyFramesHandler : public network::ProtocolTypeHandler<network::NodeKeyFramesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeKeyFramesHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodeKeyFramesMessage *message) override;
};

} // namespace network
} // namespace voxedit
