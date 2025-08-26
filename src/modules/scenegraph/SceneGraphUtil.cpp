/**
 * @file
 */

#include "SceneGraphUtil.h"
#include "core/Log.h"
#include <glm/ext/scalar_constants.hpp>
#include "core/collection/DynamicArray.h"
#include "math/Easing.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"

namespace scenegraph {

static int addToGraph(SceneGraph &sceneGraph, SceneGraphNode &&node, int parent) {
	if (parent > 0 && !sceneGraph.hasNode(parent)) {
		Log::error("Can't find parent node %i for %s", parent, node.name().c_str());
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
		target.setAllKeyFrames(node.allKeyFrames(), DEFAULT_ANIMATION);
	}
	target.setVisible(node.visible());
	target.setLocked(node.locked());
	target.setPivot(node.pivot());
	target.setColor(node.color());
	target.addProperties(node.properties());
	// TODO: SCENEGRAPH: the reference node id might have changed - fix this
	target.setReference(node.reference());
	if (node.hasPalette()) {
		target.setPalette(node.palette());
	}
	if (node.hasNormalPalette()) {
		target.setNormalPalette(node.normalPalette());
	}
	if (node.type() == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
	} else if (node.type() == SceneGraphNodeType::ModelReference) {
		core_assert(node.reference() != InvalidNodeId);
	} else {
		core_assert(node.volume() == nullptr);
	}
}

int createNodeReference(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent) {
	if (!node.isReferenceable()) {
		return InvalidNodeId;
	}

	SceneGraphNode newNode(SceneGraphNodeType::ModelReference);
	newNode.setReference(node.id());
	newNode.setName(node.name() + " reference");
	newNode.setColor(node.color());
	newNode.setPivot(node.pivot());
	newNode.setKeyFrames(node.keyFrames());
	if (node.hasPalette()) {
		newNode.setPalette(node.palette());
	}
	if (node.hasNormalPalette()) {
		newNode.setNormalPalette(node.normalPalette());
	}
	const int mainNodeId = addToGraph(sceneGraph, core::move(newNode), parent < 0 ? node.parent() : parent);
	if (mainNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene graph");
		return InvalidNodeId;
	}
	for (int child : node.children()) {
		const SceneGraphNode &childNode = sceneGraph.node(child);
		if (childNode.isReferenceable()) {
			createNodeReference(sceneGraph, childNode, mainNodeId);
		} else {
#if 1
			Log::warn("Don't add node %i - it is not referenceable", child);
#else
			SceneGraphNode newNode(childNode.type());
			copy(childNode, newNode);
			addToGraph(sceneGraph, core::move(newNode), mainNodeId);
#endif
		}
	}
	return mainNodeId;
}

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume, bool copyKeyFrames) {
	if (copyVolume) {
		core_assert_msg(src.volume() != nullptr, "Source node has no volume - and is of type %d", (int)src.type());
		target.setVolume(new voxel::RawVolume(src.volume()), true);
	} else if (src.isModelNode()) {
		target.setVolume(src.volume());
	}
	copy(src, target, copyKeyFrames);
}

int copyNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent, bool recursive) {
	SceneGraphNode newNode(node.type());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(node.volume()), true);
	}
	const int nodeId = addToGraph(sceneGraph, core::move(newNode), parent);
	if (recursive) {
		for (int childId : node.children()) {
			const SceneGraphNode &cnode = sceneGraph.node(childId);
			copyNodeToSceneGraph(sceneGraph, cnode, nodeId, recursive);
		}
	}
	return nodeId;
}

int moveNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent, const std::function<void(int)> &onNodeAdded) {
	SceneGraphNode newNode(node.type(), node.uuid());
	copy(node, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		core_assert(node.owns());
		newNode.setVolume(node.volume(), true);
		node.releaseOwnership();
	}
	int newNodeId = addToGraph(sceneGraph, core::move(newNode), parent);
	if (onNodeAdded && newNodeId != InvalidNodeId) {
		onNodeAdded(newNodeId);
	}
	return newNodeId;
}

static int addSceneGraphNode_r(SceneGraph &target, const SceneGraph &source, SceneGraphNode &sourceNode, int parent, const std::function<void(int)> &onNodeAdded) {
	const int newNodeId = moveNodeToSceneGraph(target, sourceNode, parent);
	if (newNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene graph");
		return 0;
	}

	int nodesAdded = sourceNode.type() == SceneGraphNodeType::Model ? 1 : 0;
	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		nodesAdded += addSceneGraphNode_r(target, source, sourceChildNode, newNodeId, onNodeAdded);
	}

	return nodesAdded;
}

int addSceneGraphNodes(SceneGraph &target, SceneGraph &source, int parent, const std::function<void(int)> &onNodeAdded) {
	const SceneGraphNode &sourceRoot = source.root();
	int nodesAdded = 0;
	target.node(parent).addProperties(sourceRoot.properties());

	for (const core::String &animation : source.animations()) {
		target.addAnimation(animation);
	}

	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded += addSceneGraphNode_r(target, source, source.node(sourceNodeId), parent, onNodeAdded);
	}
	return nodesAdded;
}

/**
 * @return the main node id that was added
 */
static int copySceneGraphNode_r(SceneGraph &target, const SceneGraph &source, const SceneGraphNode &sourceNode, int parent) {
	SceneGraphNode newNode(sourceNode.type());
	copy(sourceNode, newNode);
	if (newNode.type() == SceneGraphNodeType::Model) {
		newNode.setVolume(new voxel::RawVolume(sourceNode.volume()), true);
	}
	const int newNodeId = addToGraph(target, core::move(newNode), parent);
	if (newNodeId == InvalidNodeId) {
		Log::error("Failed to add node to the scene graph");
		return InvalidNodeId;
	}

	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		copySceneGraphNode_r(target, source, sourceChildNode, newNodeId);
	}

	return newNodeId;
}

core::Buffer<int> copySceneGraph(SceneGraph &target, const SceneGraph &source, int parent) {
	const SceneGraphNode &sourceRoot = source.root();
	core::Buffer<int> nodesAdded;

	for (const core::String &animation : source.animations()) {
		target.addAnimation(animation);
	}

	target.node(parent).addProperties(sourceRoot.properties());
	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded.push_back(copySceneGraphNode_r(target, source, source.node(sourceNodeId), parent));
	}

	for (int nodeId : nodesAdded) {
		SceneGraphNode &node = target.node(nodeId);
		if (node.type() == SceneGraphNodeType::ModelReference) {
			// this is not enough of course - the id might have already existed in the target scene graph
			if (!target.hasNode(node.reference())) {
				Log::warn("Reference node %i is not in the scene graph", node.reference());
			}
		}
	}

	// TODO: SCENEGRAPH: fix references - see copy() above
	return nodesAdded;
}

// TODO: SCENEGRAPH: split is destroying groups
// TODO: SCENEGRAPH: for referenced nodes we should have to create new model references for each newly splitted model node, too
bool splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph, bool crop,
				  bool createEmpty, bool skipHidden, const glm::ivec3 &maxSize) {
	core_assert(&srcSceneGraph != &destSceneGraph);
	destSceneGraph.reserve(srcSceneGraph.size());
	for (auto iter = srcSceneGraph.beginModel(); iter != srcSceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (skipHidden && !node.visible()) {
			continue;
		}
		const voxel::Region &region = node.region();
		if (!region.isValid()) {
			Log::warn("invalid region for node %i", node.id());
			continue;
		}
		if (glm::all(glm::lessThanEqual(region.getDimensionsInVoxels(), maxSize))) {
			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			copyNode(node, newNode, true);
			destSceneGraph.emplace(core::move(newNode));
			Log::debug("No split needed for node '%s'", node.name().c_str());
			continue;
		}
		Log::debug("Split needed for node '%s'", node.name().c_str());
		core::Buffer<voxel::RawVolume *> rawVolumes = voxelutil::splitVolume(node.volume(), maxSize, createEmpty);
		Log::debug("Created %i volumes", (int)rawVolumes.size());
		for (voxel::RawVolume *v : rawVolumes) {
			if (v == nullptr) {
				continue;
			}
			scenegraph::SceneGraphNode newNode(SceneGraphNodeType::Model);
			if (crop) {
				if (voxel::RawVolume *cv = voxelutil::cropVolume(v)) {
					delete v;
					v = cv;
				}
			}
			copyNode(node, newNode, false);
			newNode.setVolume(v, true);
			destSceneGraph.emplace(core::move(newNode));
		}
	}
	return !destSceneGraph.empty();
}

double interpolate(InterpolationType interpolationType, double current, double start, double end) {
	if (glm::abs(start - end) < glm::epsilon<double>()) {
		return start;
	}
	double val = 0.0f;
	switch (interpolationType) {
	case InterpolationType::Instant:
		val = util::easing::full(current, start, end);
		break;
	case InterpolationType::Linear:
		val = util::easing::linear(current, start, end);
		break;
	case InterpolationType::QuadEaseIn:
		val = util::easing::quadIn(current, start, end);
		break;
	case InterpolationType::QuadEaseOut:
		val = util::easing::quadOut(current, start, end);
		break;
	case InterpolationType::QuadEaseInOut:
		val = util::easing::quadInOut(current, start, end);
		break;
	case InterpolationType::CubicEaseIn:
		val = util::easing::cubicIn(current, start, end);
		break;
	case InterpolationType::CubicEaseOut:
		val = util::easing::cubicOut(current, start, end);
		break;
	case InterpolationType::CubicEaseInOut:
		val = util::easing::cubicInOut(current, start, end);
		break;
	case InterpolationType::CubicBezier:
		val = util::easing::cubicBezier(current, start, end);
		break;
	case InterpolationType::CatmullRom: {
		// TODO: SCENEGRAPH: check the parameters here
		const double t = (end - start) / current;
		val = util::easing::catmullRom(start, start, end, end, t);
		break;
	}
	case InterpolationType::Max:
		break;
	}
	return start + val;
}

} // namespace voxel
