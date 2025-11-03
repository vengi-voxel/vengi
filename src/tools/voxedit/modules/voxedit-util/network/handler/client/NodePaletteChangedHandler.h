/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePaletteChangedMessage.h"

namespace voxedit {

class SceneManager;

class NodePaletteChangedHandler : public ProtocolTypeHandler<NodePaletteChangedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePaletteChangedHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, NodePaletteChangedMessage *message) override;
};

} // namespace voxedit
