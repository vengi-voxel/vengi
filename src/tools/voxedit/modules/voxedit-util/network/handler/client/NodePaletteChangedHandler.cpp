/**
 * @file
 */

#include "NodePaletteChangedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
namespace network {

NodePaletteChangedHandler::NodePaletteChangedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodePaletteChangedHandler::execute(const network::ClientId &, network::NodePaletteChangedMessage *message) {
	const core::String &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		Log::warn("Received palette changed for unknown node UUID %s", uuid.c_str());
		return;
	}

	if (!node->isModelNode()) {
		Log::warn("Received palette changed for non-model node UUID %s", uuid.c_str());
		return;
	}

	const palette::Palette &palette = message->palette();

	Client &client = _sceneMgr->client();
	client.lockListener();

	node->setPalette(palette);

	client.unlockListener();
}

} // namespace network
} // namespace voxedit
