/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodeNormalPaletteChangedMessage.h"

namespace voxedit {

class SceneManager;

class NodeNormalPaletteChangedHandler : public network::ProtocolTypeHandler<NodeNormalPaletteChangedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodeNormalPaletteChangedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, NodeNormalPaletteChangedMessage *message) override;
};

} // namespace voxedit
