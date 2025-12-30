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
#include "core/collection/DynamicMap.h"

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
	// SCENEGRAPH: the reference node id is copied as is - the caller is responsible for fixing this if needed
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
		node.setVolume(nullptr, false);
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
static int copySceneGraphNode_r(SceneGraph &target, const SceneGraph &source, const SceneGraphNode &sourceNode, int parent, core::DynamicMap<int, int> &nodeMap) {
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
	nodeMap.put(sourceNode.id(), newNodeId);

	for (int sourceNodeIdx : sourceNode.children()) {
		core_assert(source.hasNode(sourceNodeIdx));
		SceneGraphNode &sourceChildNode = source.node(sourceNodeIdx);
		copySceneGraphNode_r(target, source, sourceChildNode, newNodeId, nodeMap);
	}

	return newNodeId;
}

core::Buffer<int> copySceneGraph(SceneGraph &target, const SceneGraph &source, int parent) {
	const SceneGraphNode &sourceRoot = source.root();
	core::Buffer<int> nodesAdded;
	core::DynamicMap<int, int> nodeMap;

	for (const core::String &animation : source.animations()) {
		target.addAnimation(animation);
	}

	target.node(parent).addProperties(sourceRoot.properties());
	for (int sourceNodeId : sourceRoot.children()) {
		nodesAdded.push_back(copySceneGraphNode_r(target, source, source.node(sourceNodeId), parent, nodeMap));
	}

	for (auto entry : nodeMap) {
		int newNodeId = entry->second;
		SceneGraphNode &node = target.node(newNodeId);
		if (node.type() == SceneGraphNodeType::ModelReference) {
			int oldRefId = node.reference();
			auto iter = nodeMap.find(oldRefId);
			if (iter != nodeMap.end()) {
				node.setReference(iter->second);
			} else {
				// this is not enough of course - the id might have already existed in the target scene graph
				if (!target.hasNode(oldRefId)) {
					Log::warn("Reference node %i is not in the scene graph", oldRefId);
				}
			}
		}
	}

	return nodesAdded;
}

static void splitVolumes_r(const SceneGraph &src, SceneGraph &dest, int srcNodeId, int destParentId,
						   core::DynamicMap<int, core::Buffer<int>> &splitMap,
						   bool crop, bool createEmpty, bool skipHidden, const glm::ivec3 &maxSize) {
	if (!src.hasNode(srcNodeId)) {
		return;
	}
	const SceneGraphNode &node = src.node(srcNodeId);
	if (skipHidden && !node.visible()) {
		return;
	}

	if (node.type() == SceneGraphNodeType::Model) {
		const voxel::Region &region = node.region();
		if (!region.isValid()) {
			Log::warn("invalid region for node %i", node.id());
			return;
		}

		if (glm::all(glm::lessThanEqual(region.getDimensionsInVoxels(), maxSize))) {
			SceneGraphNode newNode(SceneGraphNodeType::Model);
			copyNode(node, newNode, true);
			int newNodeId = addToGraph(dest, core::move(newNode), destParentId);
			if (newNodeId != InvalidNodeId) {
				auto iter = splitMap.find(node.id());
				if (iter == splitMap.end()) {
					core::Buffer<int> vec;
					vec.push_back(newNodeId);
					splitMap.put(node.id(), core::move(vec));
				} else {
					iter->value.push_back(newNodeId);
				}
				for (int childId : node.children()) {
					splitVolumes_r(src, dest, childId, newNodeId, splitMap, crop, createEmpty, skipHidden, maxSize);
				}
			}
		} else {
			Log::debug("Split needed for node '%s'", node.name().c_str());
			core::Buffer<voxel::RawVolume *> rawVolumes = voxelutil::splitVolume(node.volume(), maxSize, createEmpty);
			Log::debug("Created %i volumes", (int)rawVolumes.size());
			int firstPartId = InvalidNodeId;
			for (voxel::RawVolume *v : rawVolumes) {
				if (v == nullptr) {
					continue;
				}
				SceneGraphNode newNode(SceneGraphNodeType::Model);
				if (crop) {
					if (voxel::RawVolume *cv = voxelutil::cropVolume(v)) {
						delete v;
						v = cv;
					}
				}
				copyNode(node, newNode, false);
				newNode.setVolume(v, true);
				int newNodeId = addToGraph(dest, core::move(newNode), destParentId);
				if (newNodeId != InvalidNodeId) {
					auto iter = splitMap.find(node.id());
					if (iter == splitMap.end()) {
						core::Buffer<int> vec;
						vec.push_back(newNodeId);
						splitMap.put(node.id(), core::move(vec));
					} else {
						iter->value.push_back(newNodeId);
					}
					if (firstPartId == InvalidNodeId) {
						firstPartId = newNodeId;
					}
				}
			}
			if (firstPartId != InvalidNodeId) {
				for (int childId : node.children()) {
					splitVolumes_r(src, dest, childId, firstPartId, splitMap, crop, createEmpty, skipHidden, maxSize);
				}
			}
		}
	} else {
		SceneGraphNode newNode(node.type());
		copy(node, newNode, true);
		int newNodeId = addToGraph(dest, core::move(newNode), destParentId);
		if (newNodeId != InvalidNodeId) {
			auto iter = splitMap.find(node.id());
			if (iter == splitMap.end()) {
				core::Buffer<int> vec;
				vec.push_back(newNodeId);
				splitMap.put(node.id(), core::move(vec));
			} else {
				iter->value.push_back(newNodeId);
			}
			for (int childId : node.children()) {
				splitVolumes_r(src, dest, childId, newNodeId, splitMap, crop, createEmpty, skipHidden, maxSize);
			}
		}
	}
}

bool splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph, bool crop,
				  bool createEmpty, bool skipHidden, const glm::ivec3 &maxSize) {
	core_assert(&srcSceneGraph != &destSceneGraph);
	destSceneGraph.reserve(srcSceneGraph.size());
	core::DynamicMap<int, core::Buffer<int>> splitMap;

	for (int childId : srcSceneGraph.root().children()) {
		splitVolumes_r(srcSceneGraph, destSceneGraph, childId, destSceneGraph.root().id(), splitMap, crop, createEmpty, skipHidden, maxSize);
	}

	// Fix references
	// We need to collect references first because we might add new nodes while iterating
	core::Buffer<int> referenceNodes;
	for (auto iter = destSceneGraph.begin(SceneGraphNodeType::ModelReference); iter != destSceneGraph.end(); ++iter) {
		referenceNodes.push_back((*iter).id());
	}

	for (int refNodeId : referenceNodes) {
		if (!destSceneGraph.hasNode(refNodeId)) {
			continue;
		}
		SceneGraphNode &refNode = destSceneGraph.node(refNodeId);
		int oldTargetId = refNode.reference();
		auto mapIter = splitMap.find(oldTargetId);
		if (mapIter != splitMap.end()) {
			const core::Buffer<int> &newTargets = mapIter->value;
			if (!newTargets.empty()) {
				refNode.setReference(newTargets[0]);
				for (size_t i = 1; i < newTargets.size(); ++i) {
					SceneGraphNode newRefNode(SceneGraphNodeType::ModelReference);
					copy(refNode, newRefNode, true);
					newRefNode.setReference(newTargets[i]);
					addToGraph(destSceneGraph, core::move(newRefNode), refNode.parent());
				}
			}
		}
	}

	return !destSceneGraph.empty();
}

double interpolate(InterpolationType interpolationType, double current, double start, double end) {
	if (glm::abs(start - end) < glm::epsilon<double>()) {
		return start;
	}
	if (interpolationType == InterpolationType::CatmullRom) {
		// For CatmullRom, we need 4 control points. Using start/end as the middle two points
		// and extending beyond them for smoother interpolation
		const double t = (current - start) / (end - start);
		return util::easing::catmullRom(start, start, end, end, t);
	}

	double t = 0.0f;
	switch (interpolationType) {
	case InterpolationType::Instant:
		t = util::easing::full(current, start, end);
		break;
	case InterpolationType::Linear:
		t = util::easing::linear(current, start, end);
		break;
	case InterpolationType::QuadEaseIn:
		t = util::easing::quadIn(current, start, end);
		break;
	case InterpolationType::QuadEaseOut:
		t = util::easing::quadOut(current, start, end);
		break;
	case InterpolationType::QuadEaseInOut:
		t = util::easing::quadInOut(current, start, end);
		break;
	case InterpolationType::CubicEaseIn:
		t = util::easing::cubicIn(current, start, end);
		break;
	case InterpolationType::CubicEaseOut:
		t = util::easing::cubicOut(current, start, end);
		break;
	case InterpolationType::CubicEaseInOut:
		t = util::easing::cubicInOut(current, start, end);
		break;
	case InterpolationType::CubicBezier:
		// Default control points for a smooth ease-in-out curve
		t = util::easing::cubicBezier(current, start, end, 0.1, 1.0);
		break;
	case InterpolationType::CatmullRom:
	case InterpolationType::Max:
		break;
	}
	return start + (end - start) * t;
}

} // namespace voxel
