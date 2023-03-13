/**
 * @file
 */

#include "SceneGraphNode.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "util/Easing.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "scenegraph/SceneGraph.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace scenegraph {

SceneGraphTransform::SceneGraphTransform() :
	_worldOrientation{glm::quat_identity<float, glm::defaultp>()},
	_localOrientation{glm::quat_identity<float, glm::defaultp>()} {
}

void SceneGraphTransform::setPivot(const glm::vec3 &normalizedPivot) {
	_normalizedPivot = normalizedPivot;
}

void SceneGraphTransform::setTransforms(const glm::vec3 &worldTranslation, const glm::quat &worldOrientation, const glm::vec3 &worldScale,
					const glm::vec3 &localTranslation, const glm::quat &localOrientation, const glm::vec3 &localScale) {
	_worldTranslation = worldTranslation;
	_worldOrientation = worldOrientation;
	_worldScale = worldScale;
	_localTranslation = localTranslation;
	_localOrientation = localOrientation;
	_localScale = localScale;
	_worldMat = glm::translate(_worldTranslation) * glm::mat4_cast(_worldOrientation) * glm::scale(_worldScale);
	_localMat = glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(_localScale);
	_dirty = 0u;
}

void SceneGraphTransform::setWorldTranslation(const glm::vec3 &translation) {
	if (_worldTranslation == translation) {
		return;
	}

	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldTranslation = translation;
}

void SceneGraphTransform::setWorldOrientation(const glm::quat &orientation) {
	if (_worldOrientation == orientation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldOrientation = orientation;
}

void SceneGraphTransform::setWorldScale(const glm::vec3 &scale) {
	if (_worldScale == scale) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldScale = scale;
}

void SceneGraphTransform::setWorldMatrix(const glm::mat4x4 &matrix) {
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, _worldScale, _worldOrientation, _worldTranslation, skew, perspective);
	_dirty |= DIRTY_WORLDVALUES;
}

void SceneGraphTransform::setLocalTranslation(const glm::vec3 &translation) {
	if (_localTranslation == translation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localTranslation = translation;
}

void SceneGraphTransform::setLocalOrientation(const glm::quat &orientation) {
	if (_localOrientation == orientation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localOrientation = orientation;
}

void SceneGraphTransform::setLocalScale(const glm::vec3 &scale) {
	if (_localScale == scale) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localScale = scale;
}

void SceneGraphTransform::setLocalMatrix(const glm::mat4x4 &matrix) {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, _localScale, _localOrientation, _localTranslation, skew, perspective);
	_dirty |= DIRTY_LOCALVALUES;
}

void SceneGraphTransform::lerp(const SceneGraphTransform &dest, double deltaFrameSeconds) {
	const float factor = glm::clamp((float)(deltaFrameSeconds), 0.0f, 1.0f);

	core_assert_msg((dest._dirty & DIRTY_WORLDVALUES) == 0u, "dirty world values: %u", dest._dirty);
	setWorldTranslation(glm::mix(_worldTranslation, dest._worldTranslation, factor));
	setWorldOrientation(glm::slerp(_worldOrientation, dest._worldOrientation, factor));
	setWorldScale(glm::mix(_worldScale, dest._worldScale, factor));
	_dirty = 0u;

	core_assert_msg((dest._dirty & DIRTY_LOCALVALUES) == 0u, "dirty local values: %u", dest._dirty);
	setLocalTranslation(glm::mix(_localTranslation, dest._localTranslation, factor));
	setLocalOrientation(glm::slerp(_localOrientation, dest._localOrientation, factor));
	setLocalScale(glm::mix(_localScale, dest._localScale, factor));
	_dirty = 0u;

	_normalizedPivot = glm::mix(_normalizedPivot, dest._normalizedPivot, factor);
	_worldMat = glm::translate(_worldTranslation) * glm::mat4_cast(_worldOrientation) * glm::scale(_worldScale);
	_localMat = glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(_localScale);
}

const glm::vec3 &SceneGraphTransform::pivot() const {
	return _normalizedPivot;
}

const glm::mat4x4 &SceneGraphTransform::localMatrix() const {
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "dirty flag: %u", _dirty);
	return _localMat;
}

const glm::vec3 &SceneGraphTransform::localTranslation() const {
	return _localTranslation;
}

const glm::quat &SceneGraphTransform::localOrientation() const {
	return _localOrientation;
}

const glm::vec3 &SceneGraphTransform::localScale() const {
	return _localScale;
}

const glm::mat4x4 &SceneGraphTransform::worldMatrix() const {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "dirty flag: %u", _dirty);
	return _worldMat;
}

const glm::vec3 &SceneGraphTransform::worldTranslation() const {
	return _worldTranslation;
}

const glm::quat &SceneGraphTransform::worldOrientation() const {
	return _worldOrientation;
}

const glm::vec3 &SceneGraphTransform::worldScale() const {
	return _worldScale;
}

void SceneGraphTransform::update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx) {
	if (_dirty == 0u) {
		return;
	}

	if (node.id() == InvalidNodeId) {
		Log::warn("Node not yet part of the scene graph - don't perform any update");
		return;
	}

	if (_dirty & DIRTY_WORLDVALUES) {
		core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local and world were modified");
		if (node.type() == SceneGraphNodeType::Root) {
			_localTranslation = _worldTranslation;
			_localOrientation = _worldOrientation;
			_localScale = _worldScale;
		} else {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(), node.name().c_str());
			const SceneGraphNode &parent = sceneGraph.node(parentId);
			const KeyFrameIndex keyFrameIdx = parent.keyFrameForFrame(frameIdx);
			const SceneGraphTransform &parentTransform = parent.transform(keyFrameIdx);
			_localTranslation = _worldTranslation - parentTransform.worldTranslation();
			_localOrientation = _worldOrientation * glm::conjugate(parentTransform.worldOrientation());
			_localScale = parentTransform.worldScale() * _worldScale; // TODO:
		}
		Log::debug("node %3i (%i): World transform is dirty - new local values: t(%0.2f:%0.2f:%0.2f), "
				   "r(%0.2f:%0.2f:%0.2f:%0.2f), s(%0.2f, %0.2f, %0.2f)",
				   node.id(), (int)node.type(), _localTranslation.x, _localTranslation.y, _localTranslation.z,
				   _localOrientation.x, _localOrientation.y, _localOrientation.z, _localOrientation.w,
				   _localScale.x, _localScale.y, _localScale.z);
		// now ensure that we update the local matrix
		_dirty |= DIRTY_LOCALVALUES;
		_dirty &= ~(DIRTY_WORLDVALUES | DIRTY_PARENT);
	}

	if (_dirty & DIRTY_LOCALVALUES) {
		core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "local and world were modified");
		_localMat = glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(glm::vec3(_localScale));
		_dirty &= ~DIRTY_LOCALVALUES;

		// update own world matrix
		if (node.type() == SceneGraphNodeType::Root) {
			_worldMat = _localMat;
		} else {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(), node.name().c_str());
			const SceneGraphNode &parent = sceneGraph.node(parentId);
			const KeyFrameIndex keyFrameIdx = parent.keyFrameForFrame(frameIdx);
			const glm::mat4 &parentWorldMat = parent.transform(keyFrameIdx).worldMatrix();
			_worldMat = parentWorldMat * _localMat;
		}
		setWorldMatrix(_worldMat);
		_dirty &= ~(DIRTY_WORLDVALUES | DIRTY_PARENT);
		Log::debug("node %3i (%i): Local transform is dirty - new world values: t(%0.2f:%0.2f:%0.2f), "
				   "r(%0.2f:%0.2f:%0.2f:%0.2f), s(%0.2f, %0.2f, %0.2f)",
				   node.id(), (int)node.type(), _worldTranslation.x, _worldTranslation.y, _worldTranslation.z,
				   _worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w,
				   _worldScale.x, _worldScale.y, _worldScale.z);

		// after world matrix update - inform the children
		for (int childId : node.children()) {
			SceneGraphNode &child = sceneGraph.node(childId);
			const KeyFrameIndex keyFrameIdx = child.keyFrameForFrame(frameIdx);
			SceneGraphTransform &transform = child.transform(keyFrameIdx);
			transform._dirty |= DIRTY_PARENT;
			transform.update(sceneGraph, child, frameIdx);
		}
	}

	if (_dirty & DIRTY_PARENT) {
		// update own world matrix
		if (node.type() != SceneGraphNodeType::Root) {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(), node.name().c_str());
			const SceneGraphNode &parent = sceneGraph.node(parentId);
			const KeyFrameIndex keyFrameIdx = parent.keyFrameForFrame(frameIdx);
			const glm::mat4 &parentWorldMat = parent.transform(keyFrameIdx).worldMatrix();
			_worldMat = parentWorldMat * _localMat;
		}
		setWorldMatrix(_worldMat);
		_dirty &= ~(DIRTY_WORLDVALUES | DIRTY_PARENT);
		Log::debug("node %3i (%i): Parent transform is dirty - new world values: t(%0.2f:%0.2f:%0.2f), "
				   "r(%0.2f:%0.2f:%0.2f:%0.2f), s(%0.2f, %0.2f, %0.2f)",
				   node.id(), (int)node.type(), _worldTranslation.x, _worldTranslation.y, _worldTranslation.z,
				   _worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w,
				   _worldScale.x, _worldScale.y, _worldScale.z);

		// after world matrix update - inform the children
		for (int childId : node.children()) {
			SceneGraphNode &child = sceneGraph.node(childId);
			const KeyFrameIndex keyFrameIdx = child.keyFrameForFrame(frameIdx);
			SceneGraphTransform &transform = child.transform(keyFrameIdx);
			transform._dirty |= DIRTY_PARENT;
			transform.update(sceneGraph, child, frameIdx);
		}
	}

	_dirty = 0u;
}

glm::vec3 SceneGraphTransform::apply(const glm::vec3 &pos, const glm::vec3 &size) const {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "Missing update for world matrix %i", (int)_dirty);
	return glm::vec3(_worldMat * (glm::vec4(pos, 1.0f) - glm::vec4(_normalizedPivot * size, 0.0f)));
}

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = InvalidNodeId;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = InvalidNodeId;
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
	move._id = InvalidNodeId;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = InvalidNodeId;
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
}

void SceneGraphNode::setPalette(const voxel::Palette &palette) {
	if (palette.size() <= 0) {
		return;
	}
	_palette.setValue(palette);
	_palette.value()->markDirty();
}

const voxel::Palette &SceneGraphNode::palette() const {
	if (!_palette.hasValue()) {
		voxel::Palette palette;
		palette.nippon();
		_palette.setValue(palette);
	}
	return *_palette.value();
}

voxel::Palette &SceneGraphNode::palette() {
	if (!_palette.hasValue()) {
		voxel::Palette palette;
		palette.nippon();
		_palette.setValue(palette);
	}
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

void SceneGraphNode::translate(const glm::ivec3 &v, FrameIndex frameIdx) {
	if (frameIdx == (FrameIndex)-1) {
		for (SceneGraphKeyFrame &kf : _keyFrames) {
			SceneGraphTransform &transform = kf.transform();
			transform.setWorldTranslation(transform.worldTranslation() + glm::vec3(v));
		}
		return;
	}

	const KeyFrameIndex keyFrameIdx = keyFrameForFrame(frameIdx);
	SceneGraphTransform &transform = keyFrame(keyFrameIdx).transform();
	transform.setWorldTranslation(transform.worldTranslation() + glm::vec3(v));
}

bool SceneGraphNode::isLeaf() const {
	return _children.empty();
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
	if ((int)_keyFrames.size() <= keyFrameIdx) {
		_keyFrames.resize(keyFrameIdx + 1);
	}
	return _keyFrames[keyFrameIdx];
}

const SceneGraphKeyFrame& SceneGraphNode::keyFrame(KeyFrameIndex keyFrameIdx) const {
	core_assert((int)_keyFrames.size() > keyFrameIdx);
	return _keyFrames[keyFrameIdx];
}

SceneGraphTransform& SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) {
	return keyFrame(keyFrameIdx).transform();
}

const SceneGraphTransform& SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) const {
	while (keyFrameIdx > 0 && keyFrameIdx >= (int)_keyFrames.size()) {
		--keyFrameIdx;
	}
	return _keyFrames[keyFrameIdx].transform();
}

void SceneGraphNode::setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(keyFrameIdx);
	nodeFrame.setTransform(transform);
}

void SceneGraphNode::setPivot(KeyFrameIndex keyFrameIdx, const glm::ivec3 &pos, const glm::ivec3 &size) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(keyFrameIdx);
	nodeFrame.transform().setPivot(glm::vec3(pos) / glm::vec3(size));
}

const SceneGraphKeyFrames &SceneGraphNode::keyFrames() const {
	return _keyFrames;
}

KeyFrameIndex SceneGraphNode::addKeyFrame(FrameIndex frameIdx) {
	for (size_t i = 0; i < _keyFrames.size(); ++i) {
		const SceneGraphKeyFrame &kf = _keyFrames[i];
		if (kf.frameIdx == frameIdx) {
			return InvalidKeyFrame;
		}
	}

	SceneGraphKeyFrame keyFrame;
	keyFrame.frameIdx = frameIdx;
	_keyFrames.push_back(keyFrame);
	sortKeyFrames();
	return (KeyFrameIndex)(_keyFrames.size() - 1);
}

void SceneGraphNode::sortKeyFrames() {
	static auto frameSorter = [](const SceneGraphKeyFrame &a, const SceneGraphKeyFrame &b) {
		return a.frameIdx > b.frameIdx;
	};
	_keyFrames.sort(frameSorter);
}

bool SceneGraphNode::removeKeyFrame(FrameIndex frameIdx) {
	if (_keyFrames.size() <= 1) {
		return false;
	}
	const KeyFrameIndex keyFrameIdx = keyFrameForFrame(frameIdx);
	return removeKeyFrameByIndex(keyFrameIdx);
}

bool SceneGraphNode::removeKeyFrameByIndex(KeyFrameIndex keyFrameIdx) {
	if (_keyFrames.size() <= 1) {
		return false;
	}
	_keyFrames.erase(keyFrameIdx);
	return true;
}

bool SceneGraphNode::setKeyFrames(const SceneGraphKeyFrames &kf) {
	if (kf.empty()) {
		return false;
	}
	_keyFrames = kf;
	return true;
}

KeyFrameIndex SceneGraphNode::keyFrameForFrame(FrameIndex frameIdx) const {
	// this assumes that the key frames are sorted after their frame
	const int n = (int)_keyFrames.size();
	core_assert(n > 0)	;
	for (int i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = _keyFrames[i];
		if (kf.frameIdx == frameIdx) {
			return i;
		} else if (kf.frameIdx > frameIdx) {
			if (i == 0) {
				break;
			}
			return i - 1;
		}
	}
	return n - 1;
}

SceneGraphTransform SceneGraphNode::transformForFrame(FrameIndex frameIdx) const {
	// TODO ik solver https://github.com/mgerhardy/vengi/issues/182
	const SceneGraphTransform *source = nullptr;
	const SceneGraphTransform *target = nullptr;
	FrameIndex startFrameIdx = 0;
	FrameIndex endFrameIdx = 0;
	InterpolationType interpolationType = InterpolationType::Linear;

	for (const SceneGraphKeyFrame &kf : _keyFrames) {
		if (kf.frameIdx <= frameIdx) {
			source = &kf.transform();
			startFrameIdx = kf.frameIdx;
			interpolationType = kf.interpolation;
		}
		if (kf.frameIdx > frameIdx && !target) {
			target = &kf.transform();
			endFrameIdx = kf.frameIdx;
		}
		if (source && target) {
			break;
		}
	}

	if (source == nullptr) {
		return transform(0);
	}
	if (target == nullptr) {
		return *source;
	}

	double deltaFrameSeconds = 0.0f;
	switch (interpolationType) {
	case InterpolationType::Instant:
		deltaFrameSeconds = util::easing::full((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::Linear:
		deltaFrameSeconds = util::easing::linear((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::QuadEaseIn:
		deltaFrameSeconds = util::easing::quadIn((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::QuadEaseOut:
		deltaFrameSeconds = util::easing::quadOut((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::QuadEaseInOut:
		deltaFrameSeconds = util::easing::quadInOut((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::CubicEaseIn:
		deltaFrameSeconds = util::easing::cubicIn((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::CubicEaseOut:
		deltaFrameSeconds = util::easing::cubicOut((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::CubicEaseInOut:
		deltaFrameSeconds = util::easing::cubicInOut((float)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
		break;
	case InterpolationType::Max:
		deltaFrameSeconds = 0.0;
		break;
	}
	scenegraph::SceneGraphTransform transform = *source;
	transform.lerp(*target, deltaFrameSeconds);
	return transform;
}

FrameIndex SceneGraphNode::maxFrame() const {
	FrameIndex maxFrameIdx = 0;
	for (const auto &keyframe : _keyFrames) {
		maxFrameIdx = core_max(keyframe.frameIdx, maxFrameIdx);
	}
	return maxFrameIdx;
}

SceneGraphNodeCamera::SceneGraphNodeCamera() : Super(SceneGraphNodeType::Camera) {
}

float SceneGraphNodeCamera::farPlane() const {
	return propertyf(PropFarPlane);
}

void SceneGraphNodeCamera::setFarPlane(float val) {
	setProperty(PropFarPlane, core::string::toString(val));
}

float SceneGraphNodeCamera::nearPlane() const {
	return propertyf(PropNearPlane);
}

void SceneGraphNodeCamera::setNearPlane(float val) {
	setProperty(PropNearPlane, core::string::toString(val));
}

bool SceneGraphNodeCamera::isOrthographic() const {
	return property(PropMode) == Modes[0];
}

void SceneGraphNodeCamera::setOrthographic() {
	setProperty(PropMode, Modes[0]);
}

bool SceneGraphNodeCamera::isPerspective() const {
	return property(PropMode) == Modes[1];
}

void SceneGraphNodeCamera::setPerspective() {
	setProperty(PropMode, Modes[1]);
}

int SceneGraphNodeCamera::width() const {
	return property(PropWidth).toInt();
}

void SceneGraphNodeCamera::setWidth(int val) {
	setProperty(PropWidth, core::string::toString(val));
}

int SceneGraphNodeCamera::height() const {
	return property(PropHeight).toInt();
}

void SceneGraphNodeCamera::setHeight(int val) {
	setProperty(PropHeight, core::string::toString(val));
}

int SceneGraphNodeCamera::fieldOfView() const {
	return property(PropFov).toInt();
}

void SceneGraphNodeCamera::setFieldOfView(int val) {
	setProperty(PropFov, core::string::toString(val));
}

float SceneGraphNodeCamera::aspectRatio() const {
	return property(PropAspect).toFloat();
}

void SceneGraphNodeCamera::setAspectRatio(float val) {
	setProperty(PropAspect, core::string::toString(val));
}

} // namespace voxelformat
