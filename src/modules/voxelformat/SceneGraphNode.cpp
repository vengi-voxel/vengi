/**
 * @file
 */

#include "SceneGraphNode.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "util/Easing.h"
#include "voxelformat/SceneGraph.h"

#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/transform.hpp>

namespace voxelformat {

void SceneGraphTransform::setPivot(const glm::vec3 &normalizedPivot) {
	_dirty |= DIRTY_PIVOT;
	_normalizedPivot = normalizedPivot;
#if 0
	core_assert(glm::all(glm::lessThanEqual(normalizedPivot, glm::vec3(1.0f))));
	core_assert(glm::all(glm::greaterThanEqual(normalizedPivot, glm::vec3(0.0f))));
#endif
}

void SceneGraphTransform::setWorldTranslation(const glm::vec3 &translation) {
	_dirty |= DIRTY_WORLDTRANSLATION;
	_worldTranslation = translation;
}

void SceneGraphTransform::setWorldOrientation(const glm::quat &orientation) {
	_dirty |= DIRTY_WORLDORIENTATION;
	_worldOrientation = orientation;
}

void SceneGraphTransform::setWorldScale(float scale) {
	_dirty |= DIRTY_WORLDSCALE;
	_worldScale = scale;
}

void SceneGraphTransform::setWorldMatrix(const glm::mat4x4 &matrix) {
	_dirty |= DIRTY_WORLDMATRIX;
	_worldMat = matrix;
}

void SceneGraphTransform::setLocalTranslation(const glm::vec3 &translation) {
	_dirty |= DIRTY_LOCALTRANSLATION;
	_localTranslation = translation;
}

void SceneGraphTransform::setLocalOrientation(const glm::quat &orientation) {
	_dirty |= DIRTY_LOCALORIENTATION;
	_localOrientation = orientation;
}

void SceneGraphTransform::setLocalScale(float scale) {
	_dirty |= DIRTY_LOCALSCALE;
	_localScale = scale;
}

void SceneGraphTransform::setLocalMatrix(const glm::mat4x4 &matrix) {
	_dirty |= DIRTY_LOCALMATRIX;
	_localMat = matrix;
}

void SceneGraphTransform::lerp(const SceneGraphTransform &dest, double deltaFrameSeconds) {
	const float factor = glm::clamp((float)(deltaFrameSeconds), 0.0f, 1.0f);
	setWorldTranslation(glm::mix(_worldTranslation, dest._worldTranslation, factor));
	setWorldOrientation(glm::slerp(_worldOrientation, dest._worldOrientation, factor));
	setWorldScale(glm::mix(_worldScale, dest._worldScale, factor));

	setLocalTranslation(glm::mix(_localTranslation, dest._localTranslation, factor));
	setLocalOrientation(glm::slerp(_localOrientation, dest._localOrientation, factor));
	setLocalScale(glm::mix(_localScale, dest._localScale, factor));

	_worldMat = glm::translate(_worldTranslation) * glm::mat4_cast(_worldOrientation) * glm::scale(glm::vec3(_worldScale));
	_localMat = glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(glm::vec3(_localScale));
	_dirty = 0u;
}

const glm::vec3 &SceneGraphTransform::pivot() const {
	return _normalizedPivot;
}

const glm::mat4x4 &SceneGraphTransform::localMatrix() const {
	return _localMat;
}

const glm::vec3 &SceneGraphTransform::localTranslation() const {
	return _localTranslation;
}

const glm::quat &SceneGraphTransform::localOrientation() const {
	return _localOrientation;
}

float SceneGraphTransform::localScale() const {
	return _localScale;
}

const glm::mat4x4 &SceneGraphTransform::worldMatrix() const {
	return _worldMat;
}

const glm::vec3 &SceneGraphTransform::worldTranslation() const {
	return _worldTranslation;
}

const glm::quat &SceneGraphTransform::worldOrientation() const {
	return _worldOrientation;
}

float SceneGraphTransform::worldScale() const {
	return _worldScale;
}

void SceneGraphTransform::updateLocal(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx) {
	if (_dirty & DIRTY_LOCALMATRIX) {
		_dirty &= ~(DIRTY_LOCALMATRIX);
	} else if (_dirty & (DIRTY_LOCALORIENTATION | DIRTY_LOCALSCALE | DIRTY_LOCALTRANSLATION)) {
		core_assert((_dirty & DIRTY_LOCALMATRIX) == 0u);
		_dirty &= ~(DIRTY_LOCALORIENTATION | DIRTY_LOCALSCALE | DIRTY_LOCALTRANSLATION);
	}
	if (node.type() == SceneGraphNodeType::Model) {
#if 0
		const int parentId = node.parent();
		SceneGraphNode& parent = sceneGraph.node(parentId);
		if (parent.type() == SceneGraphNodeType::Model) {
		}
#endif
	}
	for (int childId : node.children()) {
		SceneGraphNode& child = sceneGraph.node(childId);
		KeyFrameIndex keyFrame = child.keyFrameForFrame(frameIdx);
		SceneGraphTransform& transform = child.transform(keyFrame);
		transform.updateLocal(sceneGraph, child, frameIdx);
	}
}

void SceneGraphTransform::updateFromWorldMatrix() {
	if (_dirty & DIRTY_PIVOT) {
		_dirty &= ~(DIRTY_PIVOT);
	}
	if (_dirty == 0u) {
		return;
	}

	core_assert((_dirty & DIRTY_WORLDMATRIX) != 0);

	_worldTranslation = _worldMat[3];
	_worldScale = glm::length(glm::vec3(_worldMat[0]));
	if (glm::abs(_worldScale) < glm::epsilon<float>()) {
		_worldScale = 1.0f;
	}
	const glm::mat3 rotMtx(glm::vec3(_worldMat[0]) / _worldScale, glm::vec3(_worldMat[1]) / _worldScale, glm::vec3(_worldMat[2]) / _worldScale);
	_worldOrientation = glm::quat_cast(rotMtx);
	_dirty &= ~(DIRTY_WORLDMATRIX);
	core_assert_msg(_dirty == 0u, "Still not clean: %u", _dirty);

	_localMat = _worldMat;
}

void SceneGraphTransform::updateWorld() {
	if (_dirty & DIRTY_WORLDMATRIX) {
		updateFromWorldMatrix();
		_dirty &= ~(DIRTY_WORLDMATRIX);
	} else if (_dirty & (DIRTY_WORLDTRANSLATION | DIRTY_WORLDORIENTATION | DIRTY_WORLDSCALE)) {
		core_assert((_dirty & DIRTY_WORLDMATRIX) == 0u);
		_worldMat = glm::translate(_worldTranslation) * glm::mat4_cast(_worldOrientation) * glm::scale(glm::vec3(_worldScale));
		_dirty &= ~(DIRTY_WORLDTRANSLATION | DIRTY_WORLDORIENTATION | DIRTY_WORLDSCALE);
	}
}

void SceneGraphTransform::update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx) {
	if (_dirty & DIRTY_PIVOT) {
		_dirty &= ~(DIRTY_PIVOT);
	}
	if (_dirty == 0u) {
		return;
	}
	updateLocal(sceneGraph, node, frameIdx);
	updateWorld();
	core_assert_msg(_dirty == 0u, "Still not clean: %u", _dirty);
}

glm::vec3 SceneGraphTransform::apply(const glm::vec3 &pos, const glm::vec3 &size) const {
	core_assert_msg((_dirty & (DIRTY_WORLDTRANSLATION | DIRTY_WORLDORIENTATION | DIRTY_WORLDSCALE)) == 0u, "Missing update for transform matrix %i", (int)_dirty);
	return glm::vec3(_worldMat * (glm::vec4(pos, 1.0f) - glm::vec4(_normalizedPivot * size, 0.0f)));
}

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = -1;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = core::move(move._keyFrames);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	move._type = SceneGraphNodeType::Max;
	_flags = move._flags;
	move._flags &= ~VolumeOwned;
}

SceneGraphNode &SceneGraphNode::operator=(SceneGraphNode &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	setVolume(move._volume, move._flags & VolumeOwned);
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = -1;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = core::move(move._keyFrames);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	_flags = move._flags;
	move._flags &= ~VolumeOwned;
	return *this;
}

SceneGraphNode::SceneGraphNode(SceneGraphNodeType type)
	: _type(type), _flags(VolumeOwned | Visible), _properties(128) {
	// ensure that there is at least one frame
	SceneGraphKeyFrame frame;
	_keyFrames.emplace_back(frame);
	_palette.setValue(&voxel::getPalette());
}

void SceneGraphNode::setPalette(const voxel::Palette &palette) {
	if (palette.size() <= 0) {
		return;
	}
	_palette.setValue(palette);
	_palette.value()->markDirty();
}

const voxel::Palette &SceneGraphNode::palette() const {
	return *_palette.value();
}

voxel::Palette &SceneGraphNode::palette() {
	return *_palette.value();
}

void SceneGraphNode::release() {
	if (_flags & VolumeOwned) {
		delete _volume;
	}
	_volume = nullptr;
}

void SceneGraphNode::releaseOwnership() {
	_flags &= ~VolumeOwned;
}

void SceneGraphNode::setVolume(voxel::RawVolume *volume, bool transferOwnership) {
	release();
	if (transferOwnership) {
		_flags |= VolumeOwned;
	} else {
		_flags &= ~VolumeOwned;
	}
	_volume = volume;
}

void SceneGraphNode::setVolume(const voxel::RawVolume *volume, bool transferOwnership) {
	release();
	if (transferOwnership) {
		_flags |= VolumeOwned;
	} else {
		_flags &= ~VolumeOwned;
	}
	_volume = (voxel::RawVolume *)volume;
}

const voxel::Region &SceneGraphNode::region() const {
	if (_volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	return _volume->region();
}

void SceneGraphNode::translate(const glm::ivec3 &v, int frame) {
	if (frame != -1) {
		const uint32_t kf = keyFrameForFrame(frame);
		SceneGraphTransform &transform = keyFrame(kf).transform();
		transform.setWorldTranslation(transform.worldTranslation() + glm::vec3(v));
	} else {
		for (SceneGraphKeyFrame &kf : _keyFrames) {
			SceneGraphTransform &transform = kf.transform();
			transform.setWorldTranslation(transform.worldTranslation() + glm::vec3(v));
		}
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

const SceneGraphNodeChildren &SceneGraphNode::children() const {
	return _children;
}

const core::StringMap<core::String> &SceneGraphNode::properties() const {
	return _properties;
}

core::StringMap<core::String> &SceneGraphNode::properties() {
	return _properties;
}

core::String SceneGraphNode::property(const core::String &key) const {
	core::String value;
	_properties.get(key, value);
	return value;
}

float SceneGraphNode::propertyf(const core::String& key) const {
	return property(key).toFloat();
}

void SceneGraphNode::addProperties(const core::StringMap<core::String> &map) {
	for (const auto &entry : map) {
		setProperty(entry->key, entry->value);
	}
}

bool SceneGraphNode::setProperty(const core::String& key, const char *value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	_properties.put(key, value);
	return true;
}

bool SceneGraphNode::setProperty(const core::String& key, bool value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	_properties.put(key, core::string::toString(value));
	return true;
}

bool SceneGraphNode::setProperty(const core::String& key, const core::String& value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	_properties.put(key, value);
	return true;
}

SceneGraphKeyFrame& SceneGraphNode::keyFrame(KeyFrameIndex keyFrameIdx) {
	if (_keyFrames.size() <= keyFrameIdx) {
		_keyFrames.resize((int)keyFrameIdx + 1);
	}
	return _keyFrames[keyFrameIdx];
}

SceneGraphTransform& SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) {
	return _keyFrames[keyFrameIdx].transform();
}

const SceneGraphTransform& SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) const {
	while (keyFrameIdx > 0 && keyFrameIdx >= _keyFrames.size()) {
		--keyFrameIdx;
	}
	return _keyFrames[keyFrameIdx].transform();
}

void SceneGraphNode::setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(keyFrameIdx);
	core_assert_msg(!transform.dirty(), "given transform is not yet ready");
	nodeFrame.setTransform(transform);
}

void SceneGraphNode::setPivot(KeyFrameIndex frameIdx, const glm::ivec3 &pos, const glm::ivec3 &size) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform().setPivot(glm::vec3(pos) / glm::vec3(size));
}

const SceneGraphKeyFrames &SceneGraphNode::keyFrames() const {
	return _keyFrames;
}

bool SceneGraphNode::addKeyFrame(FrameIndex frame) {
	for (size_t i = 0; i < _keyFrames.size(); ++i) {
		const SceneGraphKeyFrame &kf = _keyFrames[i];
		if (kf.frame == frame) {
			return false;
		}
	}

	SceneGraphKeyFrame keyFrame;
	keyFrame.frame = frame;
	_keyFrames.push_back(keyFrame);
	sortKeyFrames();
	return true;
}

void SceneGraphNode::sortKeyFrames() {
	static auto frameSorter = [](const SceneGraphKeyFrame &a, const SceneGraphKeyFrame &b) {
		return a.frame > b.frame;
	};
	_keyFrames.sort(frameSorter);
}

bool SceneGraphNode::removeKeyFrame(FrameIndex frame) {
	const uint32_t keyFrameId = keyFrameForFrame(frame);
	if (keyFrameId == 0) {
		return false;
	}
	_keyFrames.erase(keyFrameId);
	return true;
}

bool SceneGraphNode::setKeyFrames(const SceneGraphKeyFrames &kf) {
	if (kf.empty()) {
		return false;
	}
	_keyFrames = kf;
	return true;
}

uint32_t SceneGraphNode::keyFrameForFrame(FrameIndex frame) const {
	// this assumes that the key frames are sorted after their frame
	const size_t n = _keyFrames.size();
	core_assert(n > 0)	;
	for (size_t i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = _keyFrames[i];
		if (kf.frame == frame) {
			return i;
		} else if (kf.frame > frame) {
			if (i == 0) {
				break;
			}
			return i - 1;
		}
	}
	return n - 1;
}

SceneGraphTransform SceneGraphNode::transformForFrame(FrameIndex current) const {
	const SceneGraphTransform *source = nullptr;
	const SceneGraphTransform *target = nullptr;
	FrameIndex start = 0;
	FrameIndex end = 0;
	InterpolationType interpolationType = InterpolationType::Linear;

	for (const SceneGraphKeyFrame &kf : _keyFrames) {
		if (kf.frame <= current) {
			source = &kf.transform();
			start = kf.frame;
			interpolationType = kf.interpolation;
		}
		if (kf.frame > current && !target) {
			target = &kf.transform();
			end = kf.frame;
		}
		if (source && target) {
			break;
		}
	}

	if (source == nullptr || target == nullptr) {
		return transform(0);
	}

	double deltaFrameSeconds = 0.0f;
	switch (interpolationType) {
	case InterpolationType::Instant:
		deltaFrameSeconds = util::easing::full((float)current, (double)start, (double)end);
		break;
	case InterpolationType::Linear:
		deltaFrameSeconds = util::easing::linear((float)current, (double)start, (double)end);
		break;
	case InterpolationType::QuadEaseIn:
		deltaFrameSeconds = util::easing::quadIn((float)current, (double)start, (double)end);
		break;
	case InterpolationType::QuadEaseOut:
		deltaFrameSeconds = util::easing::quadOut((float)current, (double)start, (double)end);
		break;
	case InterpolationType::QuadEaseInOut:
		deltaFrameSeconds = util::easing::quadInOut((float)current, (double)start, (double)end);
		break;
	case InterpolationType::CubicEaseIn:
		deltaFrameSeconds = util::easing::cubicIn((float)current, (double)start, (double)end);
		break;
	case InterpolationType::CubicEaseOut:
		deltaFrameSeconds = util::easing::cubicOut((float)current, (double)start, (double)end);
		break;
	case InterpolationType::CubicEaseInOut:
		deltaFrameSeconds = util::easing::cubicInOut((float)current, (double)start, (double)end);
		break;
	case InterpolationType::Max:
		deltaFrameSeconds = 0.0;
		break;
	}
	voxelformat::SceneGraphTransform transform = *source;
	transform.lerp(*target, deltaFrameSeconds);
	return transform;
}

FrameIndex SceneGraphNode::maxFrame() const {
	FrameIndex maxFrame = 0;
	for (const auto &keyframe : _keyFrames) {
		maxFrame = core_max(keyframe.frame, maxFrame);
	}
	return maxFrame;
}

SceneGraphNodeCamera::SceneGraphNodeCamera() : Super(SceneGraphNodeType::Camera) {
}

float SceneGraphNodeCamera::farPlane() const {
	return propertyf("cam_farplane");
}

void SceneGraphNodeCamera::setFarPlane(float val) {
	setProperty("cam_farplane", core::string::toString(val));
}

float SceneGraphNodeCamera::nearPlane() const {
	return propertyf("cam_nearplane");
}

void SceneGraphNodeCamera::setNearPlane(float val) {
	setProperty("cam_nearplane", core::string::toString(val));
}

bool SceneGraphNodeCamera::isOrthographic() const {
	return property("cam_mode") == "orthographic";
}

void SceneGraphNodeCamera::setOrthographic() {
	setProperty("cam_mode", "orthographic");
}

bool SceneGraphNodeCamera::isPerspective() const {
	return property("cam_mode") == "perspective";
}

void SceneGraphNodeCamera::setPerspective() {
	setProperty("cam_mode", "perspective");
}

int SceneGraphNodeCamera::fieldOfView() const{
	return property("cam_fov").toInt();
}

void SceneGraphNodeCamera::setFieldOfView(int val){
	setProperty("cam_fov", core::string::toString(val));
}

} // namespace voxelformat
