/**
 * @file
 */

#include "NodeAddedHandler.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/VolumeCompression.h"

namespace voxedit {
namespace network {

NodeAddedHandler::NodeAddedHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void NodeAddedHandler::execute(const network::ClientId &, network::NodeAddedMessage *message) {
	const core::String &parentUUID = message->parentUUID();
	const core::String &nodeUUID = message->nodeUUID();
	const core::String &referenceUUID = message->referenceUUID();
	const core::String &name = message->name();
	const scenegraph::SceneGraphNodeType nodeType = message->nodeType();
	const glm::vec3 &pivot = message->pivot();

	scenegraph::SceneGraphNode *parentNode = nullptr;
	if (!parentUUID.empty()) {
		parentNode = _sceneMgr->sceneGraph().findNodeByUUID(parentUUID);
		if (parentNode == nullptr) {
			Log::warn("Received node added for unknown parent UUID %s", parentUUID.c_str());
			return;
		}
	}

	scenegraph::SceneGraphNode *existingNode = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (existingNode != nullptr) {
		Log::debug("Node with UUID %s already exists, skipping", nodeUUID.c_str());
		return;
	}

	scenegraph::SceneGraphNode newNode(nodeType, nodeUUID);
	newNode.setName(name);
	newNode.setPivot(pivot);

	// Handle reference node if specified
	if (!referenceUUID.empty()) {
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

} // namespace network
} // namespace voxedit
