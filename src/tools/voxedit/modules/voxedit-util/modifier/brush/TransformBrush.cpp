/**
 * @file
 */

#include "TransformBrush.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/trigonometric.hpp>

namespace voxedit {

TransformBrush::TransformBrush(const TransformBrush &other) : Super(other), TransformOperation(other) {
	setParameters(other.parameters());
}

void TransformBrush::setTransformMode(TransformMode mode) {
	markSettingsChanged();
	if (_parameters.mode != mode) {
		commitCurrentTransform();
		_parameters.mode = mode;
	}
}

void TransformBrush::onSceneChange() {
	Super::onSceneChange();
	commitCurrentTransform();
	_active = false;
	_snapshotHelper.clear();
}

void TransformBrush::onActivated() {
	onSceneChange();
}

void TransformBrush::commitCurrentTransform() {
	_snapshotHelper.clear();
	_cachedRegionValid = false;
	_cachedRegion = voxel::Region::InvalidRegion;
	_prevTransformedRegion = voxel::Region::InvalidRegion;
	_parameters.translation = glm::ivec3(0);
	_parameters.shear = glm::ivec3(0);
	_parameters.scale = glm::vec3(1.0f);
	_parameters.rotationDegrees = glm::vec3(0.0f);
}

void TransformBrush::reset() {
	Super::reset();
	_active = false;

	commitCurrentTransform();
	_snapshotCenter = glm::vec3(0.0f);
	_parameters.mode = TransformMode::Move;
	_parameters.sampling = voxel::VoxelSampling::Nearest;
}

bool TransformBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	_active = true;
	return true;
}

void TransformBrush::endBrush(BrushContext &) {
	_active = false;
}

bool TransformBrush::active() const {
	return _active || _snapshotHelper.hasSnapshot();
}

void TransformBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	prepare(volume, ctx.targetVolumeRegion);
}

voxel::Region TransformBrush::calcRegion(const BrushContext &ctx) const {
	return _cachedRegionValid ? _cachedRegion : ctx.targetVolumeRegion;
}

bool TransformBrush::dirtyRegions(core::Buffer<voxel::Region> &regions) const {
	return TransformOperation::dirtyRegions(regions);
}

void TransformBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &,
							  const voxel::Region &) {
	evaluate(wrapper);
	markDirty();
}

void TransformBrush::handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const {
	if (hasHandle(ctx)) {
		BrushGizmoState handle;
		describeHandle(ctx, handle);
		output.push_back(handle);
	}
}

bool TransformBrush::hasHandle(const BrushContext &ctx) const {
	return _snapshotHelper.hasSnapshot() || ctx.targetVolumeRegion.isValid();
}

void TransformBrush::describeHandle(const BrushContext &ctx, BrushGizmoState &state) const {
	state.snap[0] = (float)ctx.gridResolution.x;
	state.snap[1] = (float)ctx.gridResolution.y;
	state.snap[2] = (float)ctx.gridResolution.z;
	state.localMode = true;

	const glm::vec3 center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_parameters.mode) {
	case TransformMode::Move:
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_parameters.translation));
		break;
	case TransformMode::Rotate: {
		state.operations = BrushGizmo_Rotate;
		const glm::vec3 radians = glm::radians(_parameters.rotationDegrees);
		const glm::mat4 rot = glm::eulerAngleYXZ(radians.y, radians.x, radians.z);
		state.matrix = glm::translate(glm::mat4(1.0f), center) * rot;
		state.snap[0] = state.snap[1] = state.snap[2] = 15.0f; // 15 degree snap for rotation
		break;
	}
	case TransformMode::Scale:
		state.operations = BrushGizmo_Scale;
		state.matrix = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), _parameters.scale);
		state.snap[0] = state.snap[1] = state.snap[2] = 0.1f;
		break;
	case TransformMode::Shear:
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_parameters.shear));
		break;
	default:
		state.operations = BrushGizmo_None;
		break;
	}
}

bool TransformBrush::editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
									 uint32_t operation) {
	core_trace_scoped(ApplyBrushGizmo);
	const glm::vec3 center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_parameters.mode) {
	case TransformMode::Move: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const glm::vec3 newPos(matrix[3]);
		const glm::ivec3 offset = glm::ivec3(glm::round(newPos - center));
		if (offset != _parameters.translation) {
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
		if (offset != _parameters.shear) {
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
		if (euler != _parameters.rotationDegrees) {
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
		if (newScale != _parameters.scale) {
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
