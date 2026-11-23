/**
 * @file
 */

#include "TransformBrush.h"
#include "core/TraceScoped.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"
#include "math/QuatOps.h"
#include "math/Scalar.h"


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
	_parameters.translation = math::Vec3i(0);
	_parameters.shear = math::Vec3i(0);
	_parameters.scale = math::Vec3f(1.0f);
	_parameters.rotationDegrees = math::Vec3f(0.0f);
}

void TransformBrush::reset() {
	Super::reset();
	_active = false;

	commitCurrentTransform();
	_snapshotCenter = math::Vec3f(0.0f);
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

	const math::Vec3f center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : math::Vec3f(ctx.targetVolumeRegion.getCenter());

	switch (_parameters.mode) {
	case TransformMode::Move:
		state.operations = BrushGizmo_Translate;
		state.matrix = math::translate(math::Mat4f(1.0f), center + math::Vec3f(_parameters.translation));
		break;
	case TransformMode::Rotate: {
		state.operations = BrushGizmo_Rotate;
		const math::Vec3f radians = math::radians(_parameters.rotationDegrees);
		const math::Mat4f rot = math::eulerAngleYXZ(radians.y, radians.x, radians.z);
		state.matrix = math::translate(math::Mat4f(1.0f), center) * rot;
		state.snap[0] = state.snap[1] = state.snap[2] = 15.0f; // 15 degree snap for rotation
		break;
	}
	case TransformMode::Scale:
		state.operations = BrushGizmo_Scale;
		state.matrix = math::translate(math::Mat4f(1.0f), center) * math::scale(math::Mat4f(1.0f), _parameters.scale);
		state.snap[0] = state.snap[1] = state.snap[2] = 0.1f;
		break;
	case TransformMode::Shear:
		state.operations = BrushGizmo_Translate;
		state.matrix = math::translate(math::Mat4f(1.0f), center + math::Vec3f(_parameters.shear));
		break;
	default:
		state.operations = BrushGizmo_None;
		break;
	}
}

bool TransformBrush::editHandle(uint32_t handleId, BrushContext &ctx, const math::Mat4f &matrix, const math::Mat4f &deltaMatrix,
									 uint32_t operation) {
	core_trace_scoped(ApplyBrushGizmo);
	const math::Vec3f center = _snapshotHelper.hasSnapshot() ? _snapshotCenter : math::Vec3f(ctx.targetVolumeRegion.getCenter());

	switch (_parameters.mode) {
	case TransformMode::Move: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const math::Vec3f newPos(matrix[3]);
		const math::Vec3i offset = math::Vec3i(math::round(newPos - center));
		if (offset != _parameters.translation) {
			setMoveOffset(offset);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Shear: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const math::Vec3f newPos(matrix[3]);
		const math::Vec3i offset = math::Vec3i(math::round(newPos - center));
		if (offset != _parameters.shear) {
			setShearOffset(offset);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Rotate: {
		// Extract rotation from manipulated matrix (translation removed)
		math::Vec3f scale;
		math::Vec3f translation;
		math::Quatf orientation;
		math::Vec3f skew;
		math::Vec4f perspective;
		math::decompose(matrix, scale, orientation, translation, skew, perspective);
		const math::Vec3f euler = math::degrees(math::eulerAngles(orientation));
		if (euler != _parameters.rotationDegrees) {
			setRotationDegrees(euler);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Scale: {
		// Extract scale from manipulated matrix
		math::Vec3f newScale;
		math::Vec3f translation;
		math::Quatf orientation;
		math::Vec3f skew;
		math::Vec4f perspective;
		math::decompose(matrix, newScale, orientation, translation, skew, perspective);
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
