/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Pair.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

SceneGraph::SceneGraph(int nodes) : _nodes(nodes) {
	clear();
	_animations.push_back("Default");
}

SceneGraph::~SceneGraph() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
}

SceneGraph::SceneGraph(SceneGraph &&other) noexcept {
	_nodes = core::move(other._nodes);
	_nextNodeId = other._nextNodeId;
	other._nextNodeId = 0;
	_activeNodeId = other._activeNodeId;
	other._activeNodeId = -1;
	_animations = core::move(other._animations);
}

SceneGraph &SceneGraph::operator=(SceneGraph &&other) noexcept {
	if (this != &other) {
		_nodes = core::move(other._nodes);
		_nextNodeId = other._nextNodeId;
		other._nextNodeId = 0;
		_activeNodeId = other._activeNodeId;
		other._activeNodeId = -1;
		_animations = core::move(other._animations);
	}
	return *this;
}

const core::DynamicArray<core::String>& SceneGraph::animations() const {
	return _animations;
}

bool SceneGraph::addAnimation(const core::String &animation) {
	if (core::find(_animations.begin(), _animations.end(), animation) != _animations.end()) {
		return false;
	}
	_animations.push_back(animation);
	return true;
}

int SceneGraph::activeNode() const {
	return _activeNodeId;
}

bool SceneGraph::setActiveNode(int nodeId) {
	if (!hasNode(nodeId)) {
		return false;
	}
	_activeNodeId = nodeId;
	return true;
}

voxel::Palette &SceneGraph::firstPalette() const {
	for (SceneGraphNode& node : *this) {
		return node.palette();
	}
	return voxel::getPalette();
}

SceneGraphNode& SceneGraph::node(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::error("No node for id %i found in the scene graph - returning root node", nodeId);
		return _nodes.find(0)->value;
	}
	return iter->value;
}

bool SceneGraph::hasNode(int nodeId) const {
	return _nodes.find(nodeId) != _nodes.end();
}

const SceneGraphNode& SceneGraph::root() const {
	return node(0);
}

int SceneGraph::prevModelNode(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter ==  _nodes.end()) {
		return -1;
	}
	const SceneGraphNode &ownNode = iter->second;
	if (ownNode.parent() == -1) {
		return -1;
	}
	int lastChild = -1;
	const SceneGraphNode &parentNode = node(ownNode.parent());
	const auto &children = parentNode.children();
	for (int child : children) {
		if (child == nodeId) {
			if (lastChild == -1) {
				break;
			}
			return lastChild;
		}
		if (node(child).type() == SceneGraphNodeType::Model) {
			lastChild = child;
			continue;
		}
	}
	if (parentNode.type() == SceneGraphNodeType::Model) {
		return parentNode.id();
	}
	core_assert_msg(false, "Node %i is not part of the parent node", nodeId);
	return -1;
}

int SceneGraph::nextModelNode(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter ==  _nodes.end()) {
		return -1;
	}
	const SceneGraphNode &ownNode = iter->second;
	if (ownNode.parent() == -1) {
		return -1;
	}
	const auto &children = node(ownNode.parent()).children();
	for (int child : children) {
		if (child == nodeId) {
			continue;
		}
		if (node(child).type() == SceneGraphNodeType::Model) {
			return child;
		}
	}
	bool found = false;
	for (iterator modelIter = begin(); modelIter != end(); ++modelIter) {
		if ((*modelIter).id() == nodeId) {
			found = true;
			continue;
		}
		if (found) {
			return (*modelIter).id();
		}
	}
	return -1;
}

void SceneGraph::updateTransforms_r(SceneGraphNode &n) {
	for (int childrenId : n.children()) {
		SceneGraphNode &children = node(childrenId);
		for (auto &keyframe : children.keyFrames()) {
			keyframe.transform().update(*this, children, keyframe.frameIdx);
		}
		updateTransforms_r(children);
	}
}

void SceneGraph::updateTransforms() {
	SceneGraphNode &root = node(0);
	for (auto &keyframe : root.keyFrames()) {
		keyframe.transform().update(*this, root, keyframe.frameIdx);
	}
	updateTransforms_r(root);
}

voxel::Region SceneGraph::groupRegion() const {
	int nodeId = activeNode();
	voxel::Region region = node(nodeId).region();
	if (node(nodeId).locked()) {
		for (iterator iter = begin(SceneGraphNodeType::Model); iter != end(); ++iter) {
			if ((*iter).locked()) {
				region.accumulate((*iter).region());
			}
		}
	}
	return region;
}

voxel::Region SceneGraph::region() const {
	voxel::Region r;
	bool validVolume = false;
	for (const SceneGraphNode& node : *this) {
		if (validVolume) {
			r.accumulate(node.region());
			continue;
		}
		r = node.region();
		validVolume = true;
	}
	return r;
}

SceneGraphNode* SceneGraph::findNodeByName(const core::String& name) {
	for (const auto& entry : _nodes) {
		Log::trace("node name: %s", entry->value.name().c_str());
		if (entry->value.name() == name) {
			return &entry->value;
		}
	}
	return nullptr;
}

SceneGraphNode* SceneGraph::first() {
	for (const auto& entry : _nodes) {
		return &entry->value;
	}
	return nullptr;
}

int SceneGraph::emplace(SceneGraphNode &&node, int parent) {
	core_assert_msg((int)node.type() < (int)SceneGraphNodeType::Max, "%i", (int)node.type());
	if (node.type() == SceneGraphNodeType::Root && _nextNodeId != 0) {
		Log::error("No second root node is allowed in the scene graph");
		node.release();
		return -1;
	}
	if (node.type() == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
		core_assert(node.region().isValid());
	}
	const int nodeId = _nextNodeId;
	if (parent >= nodeId) {
		Log::error("Invalid parent id given: %i", parent);
		node.release();
		return -1;
	}
	if (parent >= 0) {
		auto parentIter = _nodes.find(parent);
		if (parentIter == _nodes.end()) {
			Log::error("Could not find parent node with id %i", parent);
			node.release();
			return -1;
		}
		Log::debug("Add child %i to node %i", nodeId, parent);
		parentIter->value.addChild(nodeId);
	}
	++_nextNodeId;
	node.setId(nodeId);
	if (_activeNodeId == -1) {
		// try to set a sane default value for the active node
		if (node.type() == SceneGraphNodeType::Model) {
			_activeNodeId = nodeId;
		}
	}
	node.setParent(parent);
	Log::debug("Adding scene graph node of type %i with id %i and parent %i", (int)node.type(), node.id(), node.parent());
	_nodes.emplace(nodeId, core::forward<SceneGraphNode>(node));
	return nodeId;
}

bool SceneGraph::nodeHasChildren(const SceneGraphNode &n, int childId) const {
	for (int c : n.children()) {
		if (c == childId) {
			return true;
		}
	}
	for (int c : n.children()) {
		if (nodeHasChildren(node(c), childId)) {
			return true;
		}
	}
	return false;
}

bool SceneGraph::canChangeParent(const SceneGraphNode &node, int newParentId) const {
	if (node.id() == root().id()) {
		return false;
	}
	if (!hasNode(newParentId)) {
		return false;
	}
	return !nodeHasChildren(node, newParentId);
}

bool SceneGraph::changeParent(int nodeId, int newParentId) {
	if (!hasNode(nodeId)) {
		return false;
	}
	SceneGraphNode& n = node(nodeId);
	if (!canChangeParent(n, newParentId)) {
		return false;
	}

	const int oldParentId = n.parent();
	if (!node(oldParentId).removeChild(nodeId)) {
		return false;
	}
	if (!node(newParentId).addChild(nodeId)) {
		node(oldParentId).addChild(nodeId);
		return false;
	}
	n.setParent(newParentId);
	const SceneGraphNode &parentNode = node(newParentId);

	for (SceneGraphKeyFrame &keyframe : n.keyFrames()) {
		SceneGraphTransform &transform = keyframe.transform();
		const SceneGraphTransform &parentFrameTransform = parentNode.transformForFrame(keyframe.frameIdx);
		const glm::vec3 &tdelta = transform.worldTranslation() - parentFrameTransform.worldTranslation();
		const glm::quat &tquat = transform.worldOrientation() - parentFrameTransform.worldOrientation();
		transform.setLocalTranslation(tdelta);
		transform.setLocalOrientation(tquat);
	}
	updateTransforms();
	return true;
}

bool SceneGraph::removeNode(int nodeId, bool recursive) {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::debug("Could not remove node %i - not found", nodeId);
		return false;
	}
	if (iter->value.type() == SceneGraphNodeType::Root) {
		core_assert(nodeId == 0);
		clear();
		return true;
	}
	bool state = true;
	const int parent = iter->value.parent();
	SceneGraphNode &parentNode = node(parent);
	parentNode.removeChild(nodeId);

	if (recursive) {
		state = iter->value.children().empty();
		for (int childId : iter->value.children()) {
			state |= removeNode(childId, recursive);
		}
	} else {
		// reparent any children
		for (int childId : iter->value.children()) {
			node(childId).setParent(parent);
			parentNode.addChild(childId);
		}
	}
	_nodes.erase(iter);
	if (_activeNodeId == nodeId) {
		if (!empty(SceneGraphNodeType::Model)) {
			// get the first model node
			_activeNodeId = (*begin()).id();
		} else {
			_activeNodeId = root().id();
		}
	}
	return state;
}

void SceneGraph::reserve(size_t size) {
}

bool SceneGraph::empty(SceneGraphNodeType type) const {
	for (const auto& entry : _nodes) {
		if (entry->value.type() == type) {
			return false;
		}
	}
	return true;
}

size_t SceneGraph::size(SceneGraphNodeType type) const {
	size_t n = 0;
	for (const auto& entry : _nodes) {
		if (entry->value.type() == type) {
			++n;
		}
	}
	return n;
}

void SceneGraph::clear() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
	_nextNodeId = 1;

	SceneGraphNode node(SceneGraphNodeType::Root);
	node.setName("root");
	node.setId(0);
	node.setParent(-1);
	_nodes.emplace(0, core::move(node));
}

const SceneGraphNode *SceneGraph::operator[](int modelIdx) const {
	for (iterator iter = begin(SceneGraphNodeType::Model); iter != end(); ++iter) {
		if (modelIdx == 0) {
			return &*iter;
		}
		--modelIdx;
	}
	Log::error("Could not find scene graph node for model id %i", modelIdx);
	return nullptr;
}

SceneGraphNode *SceneGraph::operator[](int modelIdx) {
	for (iterator iter = begin(SceneGraphNodeType::Model); iter != end(); ++iter) {
		if (modelIdx == 0) {
			return &*iter;
		}
		--modelIdx;
	}
	Log::error("Could not find scene graph node for model id %i", modelIdx);
	return nullptr;
}

voxel::Palette SceneGraph::mergePalettes(bool removeUnused, int emptyIndex) const {
	voxel::Palette palette;
	bool tooManyColors = false;
	for (const SceneGraphNode &node : *this) {
		const voxel::Palette &nodePalette = node.palette();
		for (int i = 0; i < nodePalette.colorCount(); ++i) {
			const core::RGBA rgba = nodePalette.color(i);
			if (palette.hasColor(rgba)) {
				continue;
			}
			uint8_t index = 0;
			int skipIndex = rgba.a == 0 ? -1 : emptyIndex;
			if (!palette.addColorToPalette(rgba, false, &index, false, skipIndex)) {
				if (index < palette.colorCount() - 1) {
					tooManyColors = true;
					break;
				}
			}
			if (nodePalette.hasGlow(i)) {
				palette.setGlow(index, 1.0f);
			}
		}
		if (tooManyColors) {
			break;
		}
	}
	if (tooManyColors) {
		Log::debug("too many colors - restart, but skip similar");
		palette.setSize(0);
		for (int i = 0; i < voxel::PaletteMaxColors; ++i) {
			palette.removeGlow(i);
		}
		for (const SceneGraphNode &node : *this) {
			core::Array<bool, voxel::PaletteMaxColors> used;
			if (removeUnused) {
				used.fill(false);
				voxelutil::visitVolume(*node.volume(), [&used] (int, int, int, const voxel::Voxel &voxel) {
					used[voxel.getColor()] = true;
				});
			} else {
				used.fill(true);
			}
			const voxel::Palette &nodePalette = node.palette();
			for (int i = 0; i < nodePalette.colorCount(); ++i) {
				if (!used[i]) {
					Log::trace("color %i not used, skip it for this node", i);
					continue;
				}
				uint8_t index = 0;
				const core::RGBA rgba = nodePalette.color(i);
				int skipIndex = rgba.a == 0 ? -1 : emptyIndex;
				if (palette.addColorToPalette(rgba, true, &index, true, skipIndex)) {
					if (nodePalette.hasGlow(i)) {
						palette.setGlow(index, true);
					}
				}
			}
		}
	}
	palette.markDirty();
	return palette;
}

SceneGraph::MergedVolumePalette SceneGraph::merge(bool transform) const {
	const size_t n = size();
	if (n == 0) {
		return MergedVolumePalette{};
	} else if (n == 1) {
		for (SceneGraphNode& node : *this) {
			return MergedVolumePalette{new voxel::RawVolume(node.volume()), node.palette()};
		}
	}

	core::DynamicArray<const SceneGraphNode *> nodes;
	nodes.reserve(n);

	voxel::Region mergedRegion = voxel::Region::InvalidRegion;
	const voxel::Palette &palette = mergePalettes(true);

	for (const SceneGraphNode &node : *this) {
		nodes.push_back(&node);

		const glm::vec3 &translation = node.transform(0).worldTranslation();
		voxel::Region region = node.region();
		region.shift(translation);
		if (mergedRegion.isValid()) {
			mergedRegion.accumulate(region);
		} else {
			mergedRegion = region;
		}
	}

	voxel::RawVolume* merged = new voxel::RawVolume(mergedRegion);
	for (size_t i = 0; i < nodes.size(); ++i) {
		const SceneGraphNode* node = nodes[i];
		const voxel::Region& sourceRegion = node->region();
		voxel::Region destRegion = sourceRegion;
		if (transform) {
			const KeyFrameIndex keyFrameIdx = 0;
			const SceneGraphTransform &transform = node->transform(keyFrameIdx);
			const glm::vec3 &translation = transform.worldTranslation();
			destRegion.shift(translation);
			// TODO: rotation
		}

		voxelutil::mergeVolumes(merged, node->volume(), destRegion, sourceRegion, [node, &palette] (voxel::Voxel& voxel) {
			if (isAir(voxel.getMaterial())) {
				return false;
			}
			const core::RGBA color = node->palette().color(voxel.getColor());
			const uint8_t index = palette.getClosestMatch(color);
			voxel.setColor(index);
			return true;
		});
	}
	merged->translate(-mergedRegion.getLowerCorner());
	return MergedVolumePalette{merged, palette};
}

} // namespace voxel
