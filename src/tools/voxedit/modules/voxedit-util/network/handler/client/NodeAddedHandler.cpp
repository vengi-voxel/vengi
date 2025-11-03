/**
 * @file
 */

#include "NodeAddedHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/VolumeCompression.h"

namespace voxedit {

NodeAddedHandler::NodeAddedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeAddedHandler::execute(const network::ClientId &, NodeAddedMessage *message) {
	const core::UUID &parentUUID = message->parentUUID();
	const core::UUID &nodeUUID = message->nodeUUID();
	const core::UUID &referenceUUID = message->referenceUUID();
	const core::String &name = message->name();
	const scenegraph::SceneGraphNodeType nodeType = message->nodeType();
	const glm::vec3 &pivot = message->pivot();

	scenegraph::SceneGraphNode *parentNode = nullptr;
	if (parentUUID.isValid()) {
		parentNode = _sceneMgr->sceneGraph().findNodeByUUID(parentUUID);
		if (parentNode == nullptr) {
			const core::String &uuidStr = parentUUID.str();
			Log::warn("Received node added for unknown parent UUID %s", uuidStr.c_str());
			return;
		}
	}

	scenegraph::SceneGraphNode *existingNode = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (existingNode != nullptr) {
		const core::String &uuidStr = nodeUUID.str();
		Log::debug("Node with UUID %s already exists, skipping", uuidStr.c_str());
		return;
	}

	scenegraph::SceneGraphNode newNode(nodeType, nodeUUID);
	newNode.setName(name);
	newNode.setPivot(pivot);

	// Handle reference node if specified
	if (referenceUUID.isValid()) {
		scenegraph::SceneGraphNode *referenceNode = _sceneMgr->sceneGraph().findNodeByUUID(referenceUUID);
		if (referenceNode != nullptr && referenceNode->isModelNode()) {
			newNode.setVolume(referenceNode->volume(), false);
			newNode.setPalette(referenceNode->palette());
		}
	}

	if (nodeType == scenegraph::SceneGraphNodeType::Model) {
		const uint8_t *data = message->compressedData();
		const uint32_t dataSize = message->compressedSize();
		newNode.setVolume(voxel::toVolume(data, dataSize, message->region()), true);
	}
	newNode.setPalette(message->palette());
	for (const auto & e : message->properties()) {
		newNode.setProperty(e->first, e->second);
	}
	core::String animation = _sceneMgr->sceneGraph().activeAnimation();
	newNode.setAllKeyFrames(message->keyFrames(), animation);
	Client &client = _sceneMgr->client();
	client.lockListener();
	const int parentId = parentNode ? parentNode->id() : 0;
	_sceneMgr->moveNodeToSceneGraph(newNode, parentId);
	client.unlockListener();
}

} // namespace voxedit
