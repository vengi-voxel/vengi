/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeAddedMessage.h"

namespace voxedit {

class SceneManager;

class NodeAddedHandler : public ProtocolTypeHandler<NodeAddedMessage> {
private:
	SceneManager *_sceneMgr;

public:
	NodeAddedHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodeAddedMessage *message) override;
};

} // namespace voxedit
