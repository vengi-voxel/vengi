/**
 * @file
 */

#include "SceneGraphNode.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include <glm/gtx/transform.hpp>
#include <glm/ext/scalar_constants.hpp>

namespace voxel {

SceneGraphNode::SceneGraphNode(SceneGraphNodeType type) : _type(type) {
	// ensure that there is at least one frame
	SceneGraphKeyFrame frame;
	_keyFrames.emplace_back(frame);
}

void SceneGraphTransform::print() const {
	Log::error("position: %.2f:%.2f:%.2f", position.x, position.y, position.z);
	Log::error("rot: %.2f:%.2f:%.2f:%.2f", rot.x, rot.y, rot.z, rot.w);
	Log::error("scale: %.2f", scale);
	Log::error("pivot: %.2f:%.2f:%.2f", normalizedPivot.x, normalizedPivot.y, normalizedPivot.z);
	Log::error("matrix\n%.2f %.2f %.2f %.2f\n%.2f %.2f %.2f %.2f\n%.2f %.2f %.2f %.2f\n%.2f %.2f %.2f %.2f",
			   mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3], mat[2][0],
			   mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

void SceneGraphTransform::update() {
	mat = glm::translate(position) * glm::mat4_cast(rot) * glm::scale(glm::vec3(scale));
}

glm::vec3 SceneGraphTransform::apply(const glm::vec3 &pos, const glm::vec3 &size) const {
	return glm::vec3(mat * (glm::vec4(pos, 1.0f) - glm::vec4(normalizedPivot * size, 0.0f)));
}

void SceneGraphTransform::updateFromMat() {
	position = mat[3];
	scale = glm::length(glm::vec3(mat[0]));
	if (glm::abs(scale) < glm::epsilon<float>()) {
		scale = 1.0f;
	}
	const glm::mat3 rotMtx(glm::vec3(mat[0]) / scale, glm::vec3(mat[1]) / scale, glm::vec3(mat[2]) / scale);
	rot = glm::quat_cast(rotMtx);
}

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = move._name;
	_id = move._id;
	move._id = -1;
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = move._keyFrames;
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	move._type = SceneGraphNodeType::Max;
	_visible = move._visible;
	_locked = move._locked;
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
	move._id = -1;
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = move._keyFrames;
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	_visible = move._visible;
	_locked = move._locked;
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

bool SceneGraphNode::addChild(int id) {
	for (const int childId : _children) {
		if (childId == id) {
			return false;
		}
	}
	_children.push_back(id);
	return true;
}

bool SceneGraphNode::removeChild(int id) {
	const int n = (int)_children.size();
	for (int i = 0; i < n; ++i) {
		if (_children[i] == id) {
			_children.erase(i);
			return true;
		}
	}
	return false;
}

const core::Buffer<int, 32> &SceneGraphNode::children() const {
	return _children;
}

const core::StringMap<core::String> &SceneGraphNode::properties() const {
	return _properties;
}

core::StringMap<core::String> &SceneGraphNode::properties() {
	return _properties;
}

core::String SceneGraphNode::property(const core::String& key) const {
	core::String value;
	_properties.get(key, value);
	return value;
}

void SceneGraphNode::addProperties(const core::StringMap<core::String>& map) {
	for (const auto& entry : map) {
		setProperty(entry->key, entry->value);
	}
}

void SceneGraphNode::setTransform(uint8_t frameIdx, const SceneGraphTransform &transform, bool updateMatrix) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform = transform;
	if (updateMatrix) {
		nodeFrame.transform.update();
	}
}

void SceneGraphNode::setPivot(uint8_t frameIdx, const glm::ivec3 &pos, const glm::ivec3 &size) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform.normalizedPivot = glm::vec3(pos) / glm::vec3(size);
}

void SceneGraphNode::setNormalizedPivot(uint8_t frameIdx, const glm::vec3 &pivot) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform.normalizedPivot = pivot;
}

const core::DynamicArray<SceneGraphKeyFrame>& SceneGraphNode::keyFrames() const {
	return _keyFrames;
}

void SceneGraphNode::setKeyFrames(const core::DynamicArray<SceneGraphKeyFrame>& kf) {
	_keyFrames = kf;
}


} // namespace voxel
