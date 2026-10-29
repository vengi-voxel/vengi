/**
 * @file
 */

#include "TransformBrush.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/trigonometric.hpp>

namespace voxedit {

void TransformBrush::setTransformMode(TransformMode mode) {
	if (_transformMode != mode) {
		commitCurrentTransform();
		_transformMode = mode;
	}
}

void TransformBrush::onSceneChange() {
	Super::onSceneChange();
	commitCurrentTransform();
	_active = false;
	_snapshotHelper.clear();
}

void TransformBrush::onActivated() {
	reset();
	_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
}

bool TransformBrush::hasPendingChanges() const {
	return _snapshotHelper.hasSnapshot();
}

voxel::Region TransformBrush::revertChanges(voxel::RawVolume *volume) {
	return _snapshotHelper.revertChanges(volume);
}

bool TransformBrush::onDeactivated() {
	_sceneModifiedFlags = SceneModifiedFlags::All;
	return hasPendingChanges();
}

void TransformBrush::commitCurrentTransform() {
	_snapshotHelper.clear();
	_cachedRegionValid = false;
	_cachedRegion = voxel::Region::InvalidRegion;
	_prevTransformedRegion = voxel::Region::InvalidRegion;
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
}

void TransformBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;

	commitCurrentTransform();
	_snapshotCenter = glm::vec3(0.0f);
	_transformMode = TransformMode::Move;
	_voxelSampling = voxel::VoxelSampling::Nearest;
	_lastVolume = nullptr;
}

bool TransformBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	_active = true;
	return true;
}

void TransformBrush::endBrush(BrushContext &) {
	_lastVolume = nullptr;
	_active = false;
}

bool TransformBrush::active() const {
	return _active || _snapshotHelper.hasSnapshot();
}

void TransformBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_snapshotHelper.hasSnapshot() && volume != nullptr) {
		_snapshotHelper.captureSnapshot(volume, ctx.targetVolumeRegion);
		if (_snapshotHelper.hasSnapshot()) {
			const voxel::Region &sr = _snapshotHelper.snapshotRegion();
			_snapshotCenter = glm::vec3(sr.getLowerCorner() + sr.getUpperCorner()) * 0.5f;
		}
	} else if (_snapshotHelper.hasSnapshot()) {
		// Detect volume region shift (e.g. node was moved) and adjust cached data
		const glm::ivec3 delta = ctx.targetVolumeRegion.getLowerCorner() - _snapshotHelper.capturedVolumeLower();
		if (delta != glm::ivec3(0)) {
			_snapshotHelper.adjustForRegionShift(delta);
			_snapshotCenter += glm::vec3(delta);
		}
	}
	_cachedRegion = computeTransformedRegion();
	_cachedRegionValid = _cachedRegion.isValid();
}

voxel::Region TransformBrush::calcRegion(const BrushContext &ctx) const {
	if (_cachedRegionValid) {
		return _cachedRegion;
	}
	return ctx.targetVolumeRegion;
}

bool TransformBrush::dirtyRegions(core::Buffer<voxel::Region> &regions) const {
	if (!_snapshotHelper.hasSnapshot()) {
		return false;
	}
	// Report source (erased) and destination (written) regions separately
	// to avoid extracting the empty space between them.
	const voxel::Region &srcRegion = _snapshotHelper.snapshotRegion();
	const voxel::Region dstRegion = computeDestinationRegion();
	if (srcRegion.isValid()) {
		regions.push_back(srcRegion);
	}
	if (dstRegion.isValid()) {
		regions.push_back(dstRegion);
	}
	// Also include the previous frame's destination if it differs
	if (_prevTransformedRegion.isValid()) {
		regions.push_back(_prevTransformedRegion);
	}
	// Update for next frame
	_prevTransformedRegion = dstRegion;
	return !regions.empty();
}

void TransformBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							  const voxel::Region &) {
	core_trace_scoped(Generate);
	if (!_snapshotHelper.hasSnapshot()) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();
	_lastVolume = vol;

	// Restore previously transformed state before re-applying
	_snapshotHelper.restoreHistory(vol, wrapper);

	// Apply the current transform from the original snapshot
	applyTransform(wrapper);
	markDirty();
}

bool TransformBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _snapshotHelper.hasSnapshot() || ctx.targetVolumeRegion.isValid();
}

void TransformBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.snap[0] = (float)ctx.gridResolution.x;
	state.snap[1] = (float)ctx.gridResolution.y;
	state.snap[2] = (float)ctx.gridResolution.z;
	state.localMode = true;

	const glm::vec3 center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_transformMode) {
	case TransformMode::Move:
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_moveOffset));
		break;
	case TransformMode::Rotate: {
		state.operations = BrushGizmo_Rotate;
		const glm::vec3 radians = glm::radians(_rotationDegrees);
		const glm::mat4 rot = glm::eulerAngleYXZ(radians.y, radians.x, radians.z);
		state.matrix = glm::translate(glm::mat4(1.0f), center) * rot;
		state.snap[0] = state.snap[1] = state.snap[2] = 15.0f; // 15 degree snap for rotation
		break;
	}
	case TransformMode::Scale:
		state.operations = BrushGizmo_Scale;
		state.matrix = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), _scale);
		state.snap[0] = state.snap[1] = state.snap[2] = 0.1f;
		break;
	case TransformMode::Shear:
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_shearOffset));
		break;
	default:
		state.operations = BrushGizmo_None;
		break;
	}
}

bool TransformBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
									 uint32_t operation) {
	core_trace_scoped(ApplyBrushGizmo);
	const glm::vec3 center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_transformMode) {
	case TransformMode::Move: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const glm::vec3 newPos(matrix[3]);
		const glm::ivec3 offset = glm::ivec3(glm::round(newPos - center));
		if (offset != _moveOffset) {
			setMoveOffset(offset);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Shear: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const glm::vec3 newPos(matrix[3]);
		const glm::ivec3 offset = glm::ivec3(glm::round(newPos - center));
		if (offset != _shearOffset) {
			setShearOffset(offset);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Rotate: {
		// Extract rotation from manipulated matrix (translation removed)
		glm::vec3 scale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, orientation, translation, skew, perspective);
		const glm::vec3 euler = glm::degrees(glm::eulerAngles(orientation));
		if (euler != _rotationDegrees) {
			setRotationDegrees(euler);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Scale: {
		// Extract scale from manipulated matrix
		glm::vec3 newScale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, newScale, orientation, translation, skew, perspective);
		if (newScale != _scale) {
			setScale(newScale);
			markDirty();
			return true;
		}
		break;
	}
	default:
		break;
	}
	return false;
}

} // namespace voxedit
