/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Common.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"

namespace voxel {

SceneGraph::~SceneGraph() {
	_nodes.clear();
}

bool SceneGraph::emplace_back(SceneGraphNode &&node) {
	if (node.volume() == nullptr) {
		return false;
	}
	_nodes.emplace_back(core::forward<SceneGraphNode>(node));
	return true;
}

void SceneGraph::reserve(size_t size) {
	_nodes.reserve(size);
}

bool SceneGraph::empty() const {
	return _nodes.empty();
}

size_t SceneGraph::size() const {
	return _nodes.size();
}

void SceneGraph::clear() {
	for (SceneGraphNode &node : _nodes) {
		node.release();
	}
	_nodes.clear();
}

const SceneGraphNode &SceneGraph::operator[](size_t idx) const {
	return _nodes[idx];
}

SceneGraphNode &SceneGraph::operator[](size_t idx) {
	return _nodes[idx];
}

voxel::RawVolume *SceneGraph::merge() const {
	if (_nodes.empty()) {
		return nullptr;
	}
	if (_nodes.size() == 1) {
		return new voxel::RawVolume(_nodes[0].volume());
	}
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(_nodes.size());
	for (const SceneGraphNode &node : _nodes) {
		core_assert(node.volume() != nullptr);
		rawVolumes.push_back(node.volume());
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	return ::voxel::merge(rawVolumes);
}

} // namespace voxel
