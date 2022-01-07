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

void SceneGraph::emplace_back(SceneGraphNode &&node, int parent) {
	const int nodeId = _nextNodeId++;
	node.setId(nodeId);
	_nodes.emplace(nodeId, core::forward<SceneGraphNode>(node));
	if (parent >= 0) {
		auto iter = _nodes.find(parent);
		if (iter == _nodes.end()) {
			Log::error("Could not find parent node with id %i", parent);
			return;
		}
		iter->value.addChild(nodeId);
	}
}

void SceneGraph::reserve(size_t size) {
}

bool SceneGraph::empty(SceneGraphNodeType type) const {
	return begin(type) == end();
}

size_t SceneGraph::size(SceneGraphNodeType type) const {
	auto iterbegin = begin(type);
	auto iterend = end();
	size_t n = 0;
	for (auto iter = iterbegin; iter != iterend; ++iter) {
		++n;
	}
	return n;
}

void SceneGraph::clear() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
	_nextNodeId = 0;
	SceneGraphNode node(SceneGraphNodeType::Root);
	node.setName("root");
	emplace_back(core::move(node), -1);
}

const SceneGraphNode &SceneGraph::operator[](int modelIdx) const {
	iterator iter = begin(SceneGraphNodeType::Model);
	for (int i = 0; i < modelIdx; ++i) {
		core_assert(iter != end());
		++iter;
	}
	return *iter;
}

SceneGraphNode &SceneGraph::operator[](int modelIdx) {
	iterator iter = begin(SceneGraphNodeType::Model);
	for (int i = 0; i < modelIdx; ++i) {
		core_assert(iter != end());
		++iter;
	}
	return *iter;
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
