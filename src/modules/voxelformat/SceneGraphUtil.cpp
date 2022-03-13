/**
 * @file
 */

#include "SceneGraphUtil.h"
#include "core/Log.h"

namespace voxel {

int addNodeToSceneGraph(voxel::SceneGraph &sceneGraph, voxel::SceneGraphNode &node, int parent) {
	const voxel::SceneGraphNodeType type = node.type();
	const core::String name = node.name();
	voxel::SceneGraphNode newNode(type);
	newNode.setName(name);
	newNode.setKeyFrames(node.keyFrames());
	newNode.setVisible(node.visible());
	newNode.addProperties(node.properties());
	if (newNode.type() == voxel::SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
		core_assert(node.owns());
		newNode.setVolume(node.volume(), true);
		node.releaseOwnership();
	} else {
		core_assert(node.volume() == nullptr);
	}

	int newNodeId = sceneGraph.emplace(core::move(newNode), parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene");
		return -1;
	}
	return newNodeId;
}

int addSceneGraphNode_r(voxel::SceneGraph& sceneGraph, voxel::SceneGraph &newSceneGraph, voxel::SceneGraphNode &node, int parent) {
	const int newNodeId = addNodeToSceneGraph(sceneGraph, node, parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	const voxel::SceneGraphNode &newNode = newSceneGraph.node(newNodeId);
	int nodesAdded = newNode.type() == voxel::SceneGraphNodeType::Model ? 1 : 0;
	for (int nodeIdx : newNode.children()) {
		core_assert(newSceneGraph.hasNode(nodeIdx));
		voxel::SceneGraphNode &childNode = newSceneGraph.node(nodeIdx);
		nodesAdded += addSceneGraphNode_r(sceneGraph, newSceneGraph, childNode, newNodeId);
	}

	return nodesAdded;
}

int addSceneGraphNodes(voxel::SceneGraph& sceneGraph, voxel::SceneGraph& newSceneGraph, int parent) {
	const voxel::SceneGraphNode &root = newSceneGraph.root();
	int nodesAdded = 0;
	sceneGraph.node(parent).addProperties(root.properties());
	for (int nodeId : root.children()) {
		nodesAdded += addSceneGraphNode_r(sceneGraph, newSceneGraph, newSceneGraph.node(nodeId), parent);
	}
	return nodesAdded;
}

} // namespace voxel
