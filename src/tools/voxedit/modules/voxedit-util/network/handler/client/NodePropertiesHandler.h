/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePropertiesMessage.h"

namespace voxedit {

class SceneManager;

class NodePropertiesHandler : public ProtocolTypeHandler<NodePropertiesMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePropertiesHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodePropertiesMessage *message) override;
};

} // namespace voxedit
