/**
 * @file
 */

#include "SceneGraphTransform.h"
#include "core/Common.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"
#include "math/Scalar.h"


namespace scenegraph {

SceneGraphTransform::SceneGraphTransform() {
}

void SceneGraphTransform::setTransforms(const math::Vec3f &worldTranslation, const math::Quatf &worldOrientation,
										const math::Vec3f &worldScale, const math::Vec3f &localTranslation,
										const math::Quatf &localOrientation, const math::Vec3f &localScale) {
	_worldTranslation = worldTranslation;
	_worldOrientation = math::normalize(worldOrientation);
	_worldScale = worldScale;
	_localTranslation = localTranslation;
	_localOrientation = math::normalize(localOrientation);
	_localScale = localScale;
	_worldMat = math::translate(_worldTranslation) * math::mat4_cast(_worldOrientation) * math::scale(_worldScale);
	_localMat = math::translate(_localTranslation) * math::mat4_cast(_localOrientation) * math::scale(_localScale);
	_dirty = 0u;
}

void SceneGraphTransform::setWorldTranslation(const math::Vec3f &translation) {
	if (_worldTranslation == translation) {
		return;
	}

	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldTranslation = translation;
}

void SceneGraphTransform::setWorldOrientation(const math::Quatf &orientation) {
	if (_worldOrientation == orientation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldOrientation = math::normalize(orientation);
}

void SceneGraphTransform::setWorldScale(const math::Vec3f &scale) {
	if (_worldScale == scale) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	_dirty |= DIRTY_WORLDVALUES;
	_worldScale = scale;
}

void SceneGraphTransform::setWorldMatrix(const math::Mat4f &matrix) {
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "local was already modified");
	math::Vec3f skew;
	math::Vec4f perspective;
	math::Quatf orientation;
	math::decompose(matrix, _worldScale, orientation, _worldTranslation, skew, perspective);
	_worldOrientation = orientation;
	_dirty |= DIRTY_WORLDVALUES;
}

void SceneGraphTransform::setLocalTranslation(const math::Vec3f &translation) {
	if (_localTranslation == translation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localTranslation = translation;
}

void SceneGraphTransform::setLocalOrientation(const math::Quatf &orientation) {
	if (_localOrientation == orientation) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localOrientation = math::normalize(orientation);
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

void SceneGraphTransform::setLocalScale(const math::Vec3f &scale) {
	if (_localScale == scale) {
		return;
	}
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	_dirty |= DIRTY_LOCALVALUES;
	_localScale = scale;
}

void SceneGraphTransform::setLocalMatrix(const math::Mat4f &matrix) {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "world was already modified");
	math::Vec3f skew;
	math::Vec4f perspective;
	math::Quatf orientation;
	math::decompose(matrix, _localScale, orientation, _localTranslation, skew, perspective);
	_localOrientation = orientation;
	_dirty |= DIRTY_LOCALVALUES;
}

bool SceneGraphTransform::validate() const {
	if (!math::all(math::isfinite(_worldTranslation))) {
		Log::error("World translation is not finite: %f, %f, %f", _worldTranslation.x, _worldTranslation.y,
				   _worldTranslation.z);
		return false;
	}
	if (!math::all(math::isfinite(_worldScale))) {
		Log::error("World scale is not finite: %f, %f, %f", _worldScale.x, _worldScale.y, _worldScale.z);
		return false;
	}
	if (!math::all(math::isfinite(_localTranslation))) {
		Log::error("Local translation is not finite: %f, %f, %f", _localTranslation.x, _localTranslation.y,
				   _localTranslation.z);
		return false;
	}
	if (!math::all(math::isfinite(_localScale))) {
		Log::error("Local scale is not finite: %f, %f, %f", _localScale.x, _localScale.y, _localScale.z);
		return false;
	}
	if (!math::all(math::isfinite(
			math::Vec4f(_worldOrientation.x, _worldOrientation.y, _worldOrientation.z, _worldOrientation.w)))) {
		Log::error("World orientation is not finite: %f, %f, %f, %f", _worldOrientation.x, _worldOrientation.y,
				   _worldOrientation.z, _worldOrientation.w);
		return false;
	}
	if (!math::all(math::isfinite(
			math::Vec4f(_localOrientation.x, _localOrientation.y, _localOrientation.z, _localOrientation.w)))) {
		Log::error("Local orientation is not finite: %f, %f, %f, %f", _localOrientation.x, _localOrientation.y,
				   _localOrientation.z, _localOrientation.w);
		return false;
	}
	return true;
}

void SceneGraphTransform::lerp(const SceneGraphTransform &dest, double deltaFrameSeconds) {
	const float factor = math::clamp((float)(deltaFrameSeconds), 0.0f, 1.0f);

	core_assert_msg((dest._dirty & DIRTY_WORLDVALUES) == 0u, "dirty world values: %u", dest._dirty);
	setWorldTranslation(math::mix(_worldTranslation, dest._worldTranslation, factor));
	setWorldOrientation(math::slerp(_worldOrientation, dest._worldOrientation, factor));
	setWorldScale(math::mix(_worldScale, dest._worldScale, factor));
	_dirty = 0u;

	core_assert_msg((dest._dirty & DIRTY_LOCALVALUES) == 0u, "dirty local values: %u", dest._dirty);
	setLocalTranslation(math::mix(_localTranslation, dest._localTranslation, factor));
	setLocalOrientation(math::slerp(_localOrientation, dest._localOrientation, factor));
	setLocalScale(math::mix(_localScale, dest._localScale, factor));
	_dirty = 0u;

	_worldMat = math::translate(_worldTranslation) * math::mat4_cast(_worldOrientation) * math::scale(_worldScale);
	_localMat = math::translate(_localTranslation) * math::mat4_cast(_localOrientation) * math::scale(_localScale);
}

const math::Mat4f &SceneGraphTransform::localMatrix() const {
	core_assert_msg((_dirty & DIRTY_LOCALVALUES) == 0u, "dirty flag: %u", _dirty);
	return _localMat;
}

const math::Vec3f &SceneGraphTransform::localTranslation() const {
	return _localTranslation;
}

math::Quatf SceneGraphTransform::localOrientation() const {
	return _localOrientation;
}

const math::Vec3f &SceneGraphTransform::localScale() const {
	return _localScale;
}

math::Mat4f SceneGraphTransform::worldMatrix() const {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "dirty flag: %u", _dirty);
	return _worldMat;
}

const math::Vec3f &SceneGraphTransform::worldTranslation() const {
	return _worldTranslation;
}

math::Quatf SceneGraphTransform::worldOrientation() const {
	return _worldOrientation;
}

const math::Vec3f &SceneGraphTransform::worldScale() const {
	return _worldScale;
}

const math::Mat4f SceneGraphTransform::calculateLocalMatrix() const {
	return math::translate(_localTranslation) * math::mat4_cast(_localOrientation) * math::scale(math::Vec3f(_localScale));
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
			const SceneGraphNode *parent = sceneGraph.parentNode(node);
			core_assert_msg(parent != nullptr, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
			const KeyFrameIndex keyFrameIdx = parent->keyFrameForFrame(frameIdx);
			const SceneGraphTransform &parentTransform = parent->transform(keyFrameIdx);
			const math::Vec3f relativeTranslation = _worldTranslation - parentTransform.worldTranslation();
			const math::Quatf invParentOrientation = math::conjugate(parentTransform.worldOrientation());
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
		_localMat = math::translate(_localTranslation) * math::mat4_cast(_localOrientation) *
							math::scale(math::Vec3f(_localScale));
		_dirty &= ~DIRTY_LOCALVALUES;

		// update own world matrix
		if (node.type() == SceneGraphNodeType::Root) {
			_worldMat = _localMat;
		} else {
			const SceneGraphNode *parent = sceneGraph.parentNode(node);
			core_assert_msg(parent != nullptr, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
			const KeyFrameIndex keyFrameIdx = parent->keyFrameForFrame(frameIdx);
			const math::Mat4f parentWorldMat = parent->transform(keyFrameIdx).worldMatrix();
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
			for (const core::UUID &childUUID : node.children()) {
				SceneGraphNode *child = const_cast<SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
				if (child == nullptr) {
					continue;
				}
				const KeyFrameIndex keyFrameIdx = child->keyFrameForFrame(frameIdx);
				SceneGraphTransform &transform = child->transform(keyFrameIdx);
				transform._dirty |= DIRTY_WORLDVALUES;
				transform.update(sceneGraph, *child, frameIdx, true);
			}
		} else {
			// after world matrix update - inform the children
			for (const core::UUID &childUUID : node.children()) {
				SceneGraphNode *child = const_cast<SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
				if (child == nullptr) {
					continue;
				}
				const KeyFrameIndex keyFrameIdx = child->keyFrameForFrame(frameIdx);
				SceneGraphTransform &transform = child->transform(keyFrameIdx);
				transform._dirty |= DIRTY_PARENT;
				transform.update(sceneGraph, *child, frameIdx, updateChildren);
			}
		}
	}

	if (_dirty & DIRTY_PARENT) {
		// update own world matrix
		if (node.type() != SceneGraphNodeType::Root) {
			const SceneGraphNode *parent = sceneGraph.parentNode(node);
			core_assert_msg(parent != nullptr, "node %i (%s) doesn't have a root", node.id(),
							node.name().c_str());
			const KeyFrameIndex keyFrameIdx = parent->keyFrameForFrame(frameIdx);
			const math::Mat4f parentWorldMat = parent->transform(keyFrameIdx).worldMatrix();
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
		for (const core::UUID &childUUID : node.children()) {
			SceneGraphNode *child = const_cast<SceneGraphNode *>(sceneGraph.findNodeByUUID(childUUID));
			if (child == nullptr) {
				continue;
			}
			const KeyFrameIndex keyFrameIdx = child->keyFrameForFrame(frameIdx);
			SceneGraphTransform &transform = child->transform(keyFrameIdx);
			transform._dirty |= DIRTY_PARENT;
			transform.update(sceneGraph, *child, frameIdx, updateChildren);
		}
	}

	_dirty = 0u;
	return true;
}

math::Vec3f SceneGraphTransform::apply(const math::Vec3f &pos, const math::Vec3f &pivot) const {
	core_assert_msg((_dirty & DIRTY_WORLDVALUES) == 0u, "Missing update for world matrix %i", (int)_dirty);
	return math::Vec3f(_worldMat * (math::Vec4f(pos, 1.0f) - math::Vec4f(pivot, 0.0f)));
}

} // namespace scenegraph
