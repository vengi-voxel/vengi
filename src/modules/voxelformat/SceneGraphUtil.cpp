/**
 * @file
 */

#include "SceneGraphUtil.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume) {
	if (copyVolume) {
		target.setVolume(new voxel::RawVolume(src.volume()), true);
	} else {
		target.setVolume(src.volume(), false);
	}
	target.setName(src.name());
	target.setVisible(src.visible());
	target.setLocked(src.locked());
	target.addProperties(src.properties());
	target.setKeyFrames(src.keyFrames());
	target.setPalette(src.palette());
}

static int addToGraph(SceneGraph &sceneGraph, SceneGraphNode &&node, int parent) {
	int newNodeId = sceneGraph.emplace(core::move(node), parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene");
		return -1;
	}
	return newNodeId;
}

static void copy(const SceneGraphNode &node, SceneGraphNode &target) {
	target.setName(node.name());
	target.setKeyFrames(node.keyFrames());
	target.setVisible(node.visible());
	target.addProperties(node.properties());
	if (node.type() == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
	} else {
		core_assert(node.volume() == nullptr);
	}
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(node.volume()), true);
	}
	return addToGraph(sceneGraph, core::move(newNode), parent);
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		core_assert(node.owns());
		newNode.setVolume(node.volume(), true);
		node.releaseOwnership();
	}
	return addToGraph(sceneGraph, core::move(newNode), parent);
}

static int addSceneGraphNode_r(SceneGraph& sceneGraph, SceneGraph &newSceneGraph, SceneGraphNode &node, int parent) {
	const int newNodeId = addNodeToSceneGraph(sceneGraph, node, parent);
	if (newNodeId == -1) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	const SceneGraphNode &newNode = newSceneGraph.node(newNodeId);
	int nodesAdded = newNode.type() == SceneGraphNodeType::Model ? 1 : 0;
	for (int nodeIdx : newNode.children()) {
		core_assert(newSceneGraph.hasNode(nodeIdx));
		SceneGraphNode &childNode = newSceneGraph.node(nodeIdx);
		nodesAdded += addSceneGraphNode_r(sceneGraph, newSceneGraph, childNode, newNodeId);
	}

	return nodesAdded;
}

int addSceneGraphNodes(SceneGraph& sceneGraph, SceneGraph& newSceneGraph, int parent) {
	const SceneGraphNode &root = newSceneGraph.root();
	int nodesAdded = 0;
	sceneGraph.node(parent).addProperties(root.properties());
	for (int nodeId : root.children()) {
		nodesAdded += addSceneGraphNode_r(sceneGraph, newSceneGraph, newSceneGraph.node(nodeId), parent);
	}
	return nodesAdded;
}

} // namespace voxel
