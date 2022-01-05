/**
 * @file
 */

#include "SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxel {

SceneGraphNode::SceneGraphNode(voxel::RawVolume *volume, const core::String &name, bool visible)
	: _name(name), _volume(volume), _visible(visible) {
	if (volume != nullptr) {
		_pivot = volume->region().getCenter();
	} else {
		_pivot = glm::ivec3(0.0f);
	}
}

SceneGraphNode::SceneGraphNode(voxel::RawVolume *volume, const core::String &name, bool visible,
							   const glm::ivec3 &pivot)
	: _name(name), _volume(volume), _visible(visible), _pivot(pivot) {
}

SceneGraphNode::SceneGraphNode(const voxel::RawVolume *volume, const core::String &name, bool visible)
	: _name(name), _volume((voxel::RawVolume *)volume), _visible(visible) {
	if (volume != nullptr) {
		_pivot = volume->region().getCenter();
	} else {
		_pivot = glm::ivec3(0.0f);
	}
}

SceneGraphNode::SceneGraphNode(const voxel::RawVolume *volume, const core::String &name, bool visible,
							   const glm::ivec3 &pivot)
	: _name(name), _volume((voxel::RawVolume *)volume), _visible(visible), _pivot(pivot) {
}

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = move._name;
	_visible = move._visible;
	_pivot = move._pivot;
	_volumeOwned = move._volumeOwned;
	move._volumeOwned = false;
}

SceneGraphNode &SceneGraphNode::operator=(SceneGraphNode &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	setVolume(move._volume, move._volumeOwned);
	move._volume = nullptr;
	_name = move._name;
	_visible = move._visible;
	_pivot = move._pivot;
	move._volumeOwned = false;
	return *this;
}

void SceneGraphNode::release() {
	if (_volumeOwned) {
		delete _volume;
	}
	_volume = nullptr;
}

void SceneGraphNode::setVolume(voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = volume;
}

void SceneGraphNode::setVolume(const voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = (voxel::RawVolume *)volume;
}

const voxel::Region &SceneGraphNode::region() const {
	if (_volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	return _volume->region();
}

void SceneGraphNode::translate(const glm::ivec3 &v) {
	if (_volume != nullptr) {
		_volume->translate(v);
	}
}

} // namespace voxel
