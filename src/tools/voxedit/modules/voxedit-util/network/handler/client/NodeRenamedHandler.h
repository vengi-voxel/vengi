/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeRenamedMessage.h"

namespace voxedit {

class SceneManager;

class NodeRenamedHandler : public ProtocolTypeHandler<NodeRenamedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeRenamedHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodeRenamedMessage *message) override;
};

} // namespace voxedit
