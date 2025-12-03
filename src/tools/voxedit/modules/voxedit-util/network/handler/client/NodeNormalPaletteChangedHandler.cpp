/**
 * @file
 */

#include "NodeNormalPaletteChangedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/Client.h"

namespace voxedit {

NodeNormalPaletteChangedHandler::NodeNormalPaletteChangedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeNormalPaletteChangedHandler::execute(const network::ClientId &, NodeNormalPaletteChangedMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received normal palette changed for unknown node UUID %s", uuidStr.c_str());
		return;
	}

	if (!node->isModelNode()) {
		const core::String &uuidStr = uuid.str();
		Log::warn("Received normal palette changed for non-model node UUID %s", uuidStr.c_str());
		return;
	}

	const palette::NormalPalette &palette = message->palette();

	Client &client = _sceneMgr->client();
	client.lockListener();

	node->setNormalPalette(palette);

	client.unlockListener();
}

} // namespace voxedit
