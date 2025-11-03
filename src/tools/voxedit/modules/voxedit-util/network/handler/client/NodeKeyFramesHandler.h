/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeKeyFramesMessage.h"

namespace voxedit {

class SceneManager;

class NodeKeyFramesHandler : public network::ProtocolTypeHandler<NodeKeyFramesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeKeyFramesHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeKeyFramesMessage *message) override;
};

} // namespace voxedit
