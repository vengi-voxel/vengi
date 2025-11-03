/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRemovedMessage.h"

namespace voxedit {

class SceneManager;

class NodeRemovedHandler : public ProtocolTypeHandler<NodeRemovedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRemovedHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodeRemovedMessage *message) override;
};

} // namespace voxedit
