/**
 * @file
 */

#include "SceneGraphTransform.h"
#include "core/Common.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "core/Assert.h"
#include "core/Log.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/geometric.hpp>

namespace scenegraph {

SceneGraphTransform::SceneGraphTransform()
	: _worldOrientation{glm::quat_identity<float, glm::defaultp>()},
	  _localOrientation{glm::quat_identity<float, glm::defaultp>()} {
}

void SceneGraphTransform::setTransforms(const glm::vec3 &worldTranslation, const glm::quat &worldOrientation,
										const glm::vec3 &worldScale, const glm::vec3 &localTranslation,
										const glm::quat &localOrientation, const glm::vec3 &localScale) {
	_worldTranslation = worldTranslation;
	_worldOrientation = glm::normalize(worldOrientation);
	_worldScale = worldScale;
	_localTranslation = localTranslation;
	_localOrientation = glm::normalize(localOrientation);
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
	_worldOrientation = glm::normalize(orientation);
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
	_localOrientation = glm::normalize(orientation);
}

void SceneGraphTransform::mirrorX() {
	if (_dirty & DIRTY_WORLDVALUES) {
		_worldTranslation.x = -_worldTranslation.x;
		_worldScale.x = -_worldScale.x;
		_worldOrientation.x = -_worldOrientation.x;
		_worldOrientation.y = -_worldOrientation.y;
		_worldOrientation.z = -_worldOrientation.z;
	} else {
		_localTranslation.x = -_localTranslation.x;
		_localScale.x = -_localScale.x;
		_localOrientation.x = -_localOrientation.x;
		_localOrientation.y = -_localOrientation.y;
		_localOrientation.z = -_localOrientation.z;
		_dirty |= DIRTY_LOCALVALUES;
	}
}

void SceneGraphTransform::mirrorXYZ() {
	if (_dirty & DIRTY_WORLDVALUES) {
		_worldTranslation = -_worldTranslation;
		_worldScale = -_worldScale;
	} else {
		_localTranslation = -_localTranslation;
		_localScale = -_localScale;
		_dirty |= DIRTY_LOCALVALUES;
	}
}

void SceneGraphTransform::mirrorXZ() {
	if (_dirty & DIRTY_WORLDVALUES) {
		_worldTranslation.x = -_worldTranslation.x;
		_worldTranslation.z = -_worldTranslation.z;
		_worldScale.x = -_worldScale.x;
		_worldScale.z = -_worldScale.z;
	} else {
		_localTranslation.x = -_localTranslation.x;
		_localTranslation.z = -_localTranslation.z;
		_localScale.x = -_localScale.x;
		_localScale.z = -_localScale.z;
		_dirty |= DIRTY_LOCALVALUES;
	}
}

void SceneGraphTransform::rotate(math::Axis axis) {
	const int idx1 = (math::getIndexForAxis(axis) + 1) % 3;
	const int idx2 = (idx1 + 1) % 3;
	if (_dirty & DIRTY_WORLDVALUES) {
		core::exchange(_worldTranslation[idx1], _worldTranslation[idx2]);
	} else {
		core::exchange(_localTranslation[idx1], _localTranslation[idx2]);
		_dirty |= DIRTY_LOCALVALUES;
	}
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

bool SceneGraphTransform::validate() const {
	if (!glm::all(glm::isfinite(_worldTranslation))) {
		Log::error("World translation is not finite: %f, %f, %f", _worldTranslation.x, _worldTranslation.y,
				   _worldTranslation.z);
		return false;
	}
	if (!glm::all(glm::isfinite(_worldScale))) {
		Log::error("World scale is not finite: %f, %f, %f", _worldScale.x, _worldScale.y, _worldScale.z);
		return false;
	}
	if (!glm::all(glm::isfinite(_localTranslation))) {
		Log::error("Local translation is not finite: %f, %f, %f", _localTranslation.x, _localTranslation.y,
				   _localTranslation.z);
		return false;
	}
	if (!glm::all(glm::isfinite(_localScale))) {
		Log::error("Local scale is not finite: %f, %f, %f", _localScale.x, _localScale.y, _localScale.z);
		return false;
	}
	if (!glm::all(glm::isfinite(
			glm::vec4(_worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w)))) {
		Log::error("World orientation is not finite: %f, %f, %f, %f", _worldOrientation.x, _worldOrientation.y,
				   _worldOrientation.z, _worldOrientation.w);
		return false;
	}
	if (!glm::all(glm::isfinite(
			glm::vec4(_localOrientation.x, _localOrientation.y, _localOrientation.z, _localOrientation.w)))) {
		Log::error("Local orientation is not finite: %f, %f, %f, %f", _localOrientation.x, _localOrientation.y,
				   _localOrientation.z, _localOrientation.w);
		return false;
	}
	return true;
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

	_worldMat = glm::translate(_worldTranslation) * glm::mat4_cast(_worldOrientation) * glm::scale(_worldScale);
	_localMat = glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(_localScale);
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

const glm::mat4x4 SceneGraphTransform::calculateLocalMatrix() const {
	return glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(glm::vec3(_localScale));
}

bool SceneGraphTransform::update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx,
								 bool updateChildren) {
	if (_dirty == 0u) {
		return false;
	}

	if (node.id() == InvalidNodeId) {
		Log::warn("Node not yet part of the scene graph - don't perform any update");
		return false;
	}

	if (_dirty & DIRTY_WORLDVALUES) {
		core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local and world were modified");
		if (node.type() == SceneGraphNodeType::Root) {
			_localTranslation = _worldTranslation;
			_localOrientation = _worldOrientation;
			_localScale = _worldScale;
		} else {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
			const SceneGraphNode &parent = sceneGraph.node(parentId);
			const KeyFrameIndex keyFrameIdx = parent.keyFrameForFrame(frameIdx);
			const SceneGraphTransform &parentTransform = parent.transform(keyFrameIdx);
			const glm::vec3 relativeTranslation = _worldTranslation - parentTransform.worldTranslation();
			const glm::quat invParentOrientation = glm::conjugate(parentTransform.worldOrientation());
			_localTranslation = invParentOrientation * (relativeTranslation / parentTransform.worldScale());
			_localOrientation = _worldOrientation * invParentOrientation;
			_localScale = _worldScale / parentTransform.worldScale();
		}
		Log::debug("node %3i (%i): World transform is dirty - new local values: t(%0.2f:%0.2f:%0.2f), "
				   "r(%0.2f:%0.2f:%0.2f:%0.2f), s(%0.2f, %0.2f, %0.2f)",
				   node.id(), (int)node.type(), _localTranslation.x, _localTranslation.y, _localTranslation.z,
				   _localOrientation.x, _localOrientation.y, _localOrientation.z, _localOrientation.w, _localScale.x,
				   _localScale.y, _localScale.z);
		// now ensure that we update the local matrix
		_dirty |= DIRTY_LOCALVALUES;
		_dirty &= ~(DIRTY_WORLDVALUES | DIRTY_PARENT);
	}

	if (_dirty & DIRTY_LOCALVALUES) {
		core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "local and world were modified");
		_localMat =
			glm::translate(_localTranslation) * glm::mat4_cast(_localOrientation) * glm::scale(glm::vec3(_localScale));
		_dirty &= ~DIRTY_LOCALVALUES;

		// update own world matrix
		if (node.type() == SceneGraphNodeType::Root) {
			_worldMat = _localMat;
		} else {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
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
				   _worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w, _worldScale.x,
				   _worldScale.y, _worldScale.z);

		if (!updateChildren) {
			for (int childId : node.children()) {
				SceneGraphNode &child = sceneGraph.node(childId);
				const KeyFrameIndex keyFrameIdx = child.keyFrameForFrame(frameIdx);
				SceneGraphTransform &transform = child.transform(keyFrameIdx);
				transform._dirty |= DIRTY_WORLDVALUES;
				transform.update(sceneGraph, child, frameIdx, true);
			}
		} else {
			// after world matrix update - inform the children
			for (int childId : node.children()) {
				SceneGraphNode &child = sceneGraph.node(childId);
				const KeyFrameIndex keyFrameIdx = child.keyFrameForFrame(frameIdx);
				SceneGraphTransform &transform = child.transform(keyFrameIdx);
				transform._dirty |= DIRTY_PARENT;
				transform.update(sceneGraph, child, frameIdx, updateChildren);
			}
		}
	}

	if (_dirty & DIRTY_PARENT) {
		// update own world matrix
		if (node.type() != SceneGraphNodeType::Root) {
			const int parentId = node.parent();
			core_assert_msg(parentId != InvalidNodeId, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
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
				   _worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w, _worldScale.x,
				   _worldScale.y, _worldScale.z);

		// after world matrix update - inform the children
		for (int childId : node.children()) {
			SceneGraphNode &child = sceneGraph.node(childId);
			const KeyFrameIndex keyFrameIdx = child.keyFrameForFrame(frameIdx);
			SceneGraphTransform &transform = child.transform(keyFrameIdx);
			transform._dirty |= DIRTY_PARENT;
			transform.update(sceneGraph, child, frameIdx, updateChildren);
		}
	}

	_dirty = 0u;
	return true;
}

glm::vec3 SceneGraphTransform::apply(const glm::vec3 &pos, const glm::vec3 &pivot) const {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "Missing update for world matrix %i", (int)_dirty);
	return glm::vec3(_worldMat * (glm::vec4(pos, 1.0f) - glm::vec4(pivot, 0.0f)));
}

} // namespace scenegraph
