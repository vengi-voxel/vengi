/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePaletteChangedMessage.h"

namespace voxedit {

class SceneManager;

class NodePaletteChangedHandler : public network::ProtocolTypeHandler<NodePaletteChangedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePaletteChangedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodePaletteChangedMessage *message) override;
};

} // namespace voxedit
