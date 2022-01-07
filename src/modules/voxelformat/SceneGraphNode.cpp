/**
 * @file
 */

#include "SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxel {

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = move._name;
	_id = move._id;
	move._id = -1;
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	move._type = SceneGraphNodeType::Max;
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
	_id = move._id;
	_properties = move._properties;
	_children = move._children;
	_type = move._type;
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

void SceneGraphNode::releaseOwnership() {
	_volumeOwned = false;
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

void SceneGraphNode::addChild(int id) {
	_children.push_back(id);
}

const core::Buffer<int, 32> &SceneGraphNode::children() const {
	return _children;
}

const core::StringMap<core::String> &SceneGraphNode::properties() const {
	return _properties;
}

core::String SceneGraphNode::property(const core::String& key) const {
	core::String value;
	_properties.get(key, value);
	return value;
}

void SceneGraphNode::setProperty(const core::String& key, const core::String& value) {
	_properties.put(key, value);
}


} // namespace voxel
