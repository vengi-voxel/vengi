/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/NodePaletteChangedMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class NodePaletteChangedHandler : public network::ProtocolTypeHandler<network::NodePaletteChangedMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	NodePaletteChangedHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::NodePaletteChangedMessage *message) override;
};

} // namespace network
} // namespace voxedit
