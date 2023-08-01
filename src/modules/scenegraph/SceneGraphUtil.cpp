/**
 * @file
 */

#include "SceneGraphUtil.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"

namespace scenegraph {

static int addToGraph(SceneGraph &sceneGraph, SceneGraphNode &&node, int parent) {
	if (parent > 0 && !sceneGraph.hasNode(parent)) {
		parent = sceneGraph.root().id();
	}
	int newNodeId = sceneGraph.emplace(core::move(node), parent);
	if (newNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene");
		return InvalidNodeId;
	}
	return newNodeId;
}

static void copy(const SceneGraphNode &node, SceneGraphNode &target, bool copyKeyFrames = true) {
	target.setName(node.name());
	if (copyKeyFrames) {
		target.setKeyFrames(node.keyFrames());
	}
	target.setVisible(node.visible());
	target.setLocked(node.locked());
	// target.setPivot(node.pivot());
	target.setColor(node.color());
	target.addProperties(node.properties());
	// TODO: the reference node id might have changed - fix this
	target.setReference(node.reference());
	if (node.type() == SceneGraphNodeType::Model) {
		target.setPalette(node.palette());
		core_assert(node.volume() != nullptr);
	} else if (node.type() == SceneGraphNodeType::ModelReference) {
		core_assert(node.reference() != InvalidNodeId);
	} else {
		core_assert(node.volume() == nullptr);
	}
}

int createNodeReference(SceneGraph &target, const SceneGraphNode &node) {
	if (!node.isReferenceable()) {
		return InvalidNodeId;
	}

	SceneGraphNode newNode(SceneGraphNodeType::ModelReference);
	newNode.setReference(node.id());
	newNode.setName(node.name() + " reference");
	newNode.setColor(node.color());
	return addToGraph(target, core::move(newNode), node.parent());
}

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume, bool copyKeyFrames) {
	if (copyVolume) {
		core_assert_msg(src.volume() != nullptr, "Source node has no volume - and is of type %d", (int)src.type());
		target.setVolume(new voxel::RawVolume(src.volume()), true);
	} else {
		target.setVolume(src.volume(), false);
	}
	copy(src, target, copyKeyFrames);
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent, bool recursive) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(node.volume()), true);
	}
	const int nodeId = addToGraph(sceneGraph, core::move(newNode), parent);
	if (recursive) {
		for (int childId : node.children()) {
			addNodeToSceneGraph(sceneGraph, sceneGraph.node(childId), nodeId, recursive);
		}
	}
	return nodeId;
}

int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent, bool recursive) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		core_assert(node.owns());
		newNode.setVolume(node.volume(), true);
		node.releaseOwnership();
	}
	const int nodeId = addToGraph(sceneGraph, core::move(newNode), parent);
	if (recursive) {
		for (int childId : node.children()) {
			addNodeToSceneGraph(sceneGraph, sceneGraph.node(childId), nodeId, recursive);
		}
	}
	return nodeId;
}

static int addSceneGraphNode_r(SceneGraph &target, const SceneGraph &source, SceneGraphNode &sourceNode, int parent) {
	const int newNodeId = addNodeToSceneGraph(target, sourceNode, parent);
	if (newNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	int nodesAdded = sourceNode.type() == SceneGraphNodeType::Model ? 1 : 0;
	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		nodesAdded += addSceneGraphNode_r(target, source, sourceChildNode, newNodeId);
	}

	return nodesAdded;
}

int addSceneGraphNodes(SceneGraph &target, SceneGraph &source, int parent) {
	const SceneGraphNode &sourceRoot = source.root();
	int nodesAdded = 0;
	target.node(parent).addProperties(sourceRoot.properties());
	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded += addSceneGraphNode_r(target, source, source.node(sourceNodeId), parent);
	}
	return nodesAdded;
}

static int copySceneGraphNode_r(SceneGraph &target, const SceneGraph &source, const SceneGraphNode &sourceNode, int parent) {
	SceneGraphNode newNode(sourceNode.type());
	copy(sourceNode, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(sourceNode.volume()), true);
	}
	const int newNodeId = addToGraph(target, core::move(newNode), parent);
	if (newNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	int nodesAdded = sourceNode.type() == SceneGraphNodeType::Model ? 1 : 0;
	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		nodesAdded += copySceneGraphNode_r(target, source, sourceChildNode, newNodeId);
	}

	return nodesAdded;
}

int copySceneGraph(SceneGraph &target, const SceneGraph &source) {
	const SceneGraphNode &sourceRoot = source.root();
	int nodesAdded = 0;
	const int parent = target.root().id();
	target.node(parent).addProperties(sourceRoot.properties());
	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded += copySceneGraphNode_r(target, source, source.node(sourceNodeId), parent);
	}

	// TODO: fix references - see copy() above

	return nodesAdded;
}

// TODO: split is destroying groups
void splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph,
				  const glm::ivec3 &maxSize, bool crop) {
	destSceneGraph.reserve(srcSceneGraph.size());
	for (auto iter = srcSceneGraph.beginModel(); iter != srcSceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::Region &region = node.region();
		if (!region.isValid()) {
			Log::debug("invalid region for node %i", node.id());
			continue;
		}
		if (glm::all(glm::lessThanEqual(region.getDimensionsInVoxels(), maxSize))) {
			if (&srcSceneGraph != &destSceneGraph) {
				scenegraph::SceneGraphNode newNode;
				copyNode(node, newNode, true);
				destSceneGraph.emplace(core::move(newNode));
			}
			Log::debug("No split needed for node '%s'", node.name().c_str());
			continue;
		}
		Log::debug("Split needed for node '%s'", node.name().c_str());
		core::DynamicArray<voxel::RawVolume *> rawVolumes;
		voxelutil::splitVolume(node.volume(), maxSize, rawVolumes);
		for (voxel::RawVolume *v : rawVolumes) {
			scenegraph::SceneGraphNode newNode;
			if (crop) {
				voxel::RawVolume *cv = voxelutil::cropVolume(v);
				delete v;
				v = cv;
			}
			copyNode(node, newNode, false);
			newNode.setVolume(v, true);
			destSceneGraph.emplace(core::move(newNode));
		}
	}
}

} // namespace voxel
