/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeKeyFramesMessage.h"

namespace voxedit {

class SceneManager;

class NodeKeyFramesHandler : public ProtocolTypeHandler<NodeKeyFramesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeKeyFramesHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodeKeyFramesMessage *message) override;
};

} // namespace voxedit
