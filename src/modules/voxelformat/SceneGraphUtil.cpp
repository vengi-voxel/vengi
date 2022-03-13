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

} // namespace voxel