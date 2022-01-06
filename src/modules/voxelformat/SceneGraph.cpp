/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Common.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"

namespace voxel {

SceneGraph::~SceneGraph() {
	_volumes.clear();
}

bool SceneGraph::emplace_back(SceneGraphNode &&v) {
	if (v.volume() == nullptr) {
		return false;
	}
	_volumes.emplace_back(core::forward<SceneGraphNode>(v));
	return true;
}

void SceneGraph::resize(size_t size) {
	_volumes.resize(size);
}

void SceneGraph::reserve(size_t size) {
	_volumes.reserve(size);
}

bool SceneGraph::empty() const {
	return _volumes.empty();
}

size_t SceneGraph::size() const {
	return _volumes.size();
}

void SceneGraph::clear() {
	for (SceneGraphNode &v : _volumes) {
		v.release();
	}
	_volumes.clear();
}

const SceneGraphNode &SceneGraph::operator[](size_t idx) const {
	return _volumes[idx];
}

SceneGraphNode &SceneGraph::operator[](size_t idx) {
	return _volumes[idx];
}

voxel::RawVolume *SceneGraph::merge() const {
	if (_volumes.empty()) {
		return nullptr;
	}
	if (_volumes.size() == 1) {
		if (_volumes[0].volume() == nullptr) {
			return nullptr;
		}
		return new voxel::RawVolume(_volumes[0].volume());
	}
	core::DynamicArray<const voxel::RawVolume *> rawVolumes;
	rawVolumes.reserve(_volumes.size());
	for (const SceneGraphNode &v : _volumes) {
		if (v.volume() == nullptr) {
			continue;
		}
		rawVolumes.push_back(v.volume());
	}
	if (rawVolumes.empty()) {
		return nullptr;
	}
	return ::voxel::merge(rawVolumes);
}

} // namespace voxel
