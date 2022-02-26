/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Common.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeMerger.h"

namespace voxel {

SceneGraph::SceneGraph() {
	clear();
}

SceneGraph::~SceneGraph() {
	clear();
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

void SceneGraph::foreachGroup(const std::function<void(int)>& f) {
	int nodeId = activeNode();
	if (node(nodeId).locked()) {
		for (iterator iter = begin(SceneGraphNodeType::Model); iter != end(); ++iter) {
			if ((*iter).locked()) {
				f(nodeId);
			}
		}
	} else {
		f(nodeId);
	}
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

voxel::Region SceneGraph::region() const {
	voxel::Region r;
	bool validVolume = false;
	for (const voxel::SceneGraphNode& node : *this) {
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
		if (entry->value.name() == name) {
			return &entry->value;
		}
	}
	return nullptr;
}

int SceneGraph::emplace(SceneGraphNode &&node, int parent) {
	core_assert_msg((int)node.type() >= 0 && (int)node.type() < (int)voxel::SceneGraphNodeType::Max, "%i", (int)node.type());
	if (node.type() == SceneGraphNodeType::Root && _nextNodeId != 0) {
		Log::error("No second root node is allowed in the scene graph");
		node.release();
		return -1;
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

bool SceneGraph::changeParent(int nodeId, int newParentId) {
	if (nodeId == root().id()) {
		return false;
	}
	if (!hasNode(nodeId)) {
		return false;
	}
	if (!hasNode(newParentId)) {
		return false;
	}
	SceneGraphNode& n = node(nodeId);
	const int oldParentId = n.parent();
	if (!node(oldParentId).removeChild(nodeId)) {
		return false;
	}
	if (!node(newParentId).addChild(nodeId)) {
		node(oldParentId).addChild(nodeId);
		return false;
	}
	n.setParent(newParentId);
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
	if (recursive) {
		state = iter->value.children().empty();
		for (int childId : iter->value.children()) {
			state |= removeNode(childId, recursive);
		}
	} else {
		// reparent any children
		const int parent = iter->value.parent();
		SceneGraphNode &parentNode = node(parent);
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

voxel::RawVolume *SceneGraph::merge() const {
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(_nodes.size() - 1);
	for (const auto &node : *this) {
		core_assert(node.type() == SceneGraphNodeType::Model);
		core_assert(node.volume() != nullptr);
		rawVolumes.push_back(node.volume());
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	if (rawVolumes.size() == 1) {
		return new voxel::RawVolume(rawVolumes[0]);
	}
	return ::voxel::merge(rawVolumes);
}

} // namespace voxel
