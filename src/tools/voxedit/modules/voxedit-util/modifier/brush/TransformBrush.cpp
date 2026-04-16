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

voxel::Region TransformBrush::computeTransformedRegion() const {
	core_trace_scoped(ComputeTransformedRegion);
	if (!_snapshotHelper.hasSnapshot() || _snapshotHelper.snapshotVoxelCount() == 0) {
		return voxel::Region::InvalidRegion;
	}
	const glm::ivec3 &srcLo = _snapshotHelper.snapshotRegion().getLowerCorner();
	const glm::ivec3 &srcHi = _snapshotHelper.snapshotRegion().getUpperCorner();
	glm::ivec3 dstLo = srcLo;
	glm::ivec3 dstHi = srcHi;

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const glm::ivec3 cornerPos((corner & 1) ? srcHi.x : srcLo.x, (corner & 2) ? srcHi.y : srcLo.y,
								   (corner & 4) ? srcHi.z : srcLo.z);
		const glm::ivec3 transformed = transformPosition(cornerPos);
		dstLo = glm::min(dstLo, transformed);
		dstHi = glm::max(dstHi, transformed);
	}

	// Union of original and transformed regions with a small margin for rounding
	static constexpr int Margin = 2;
	const glm::ivec3 unionLo = glm::min(srcLo, dstLo) - Margin;
	const glm::ivec3 unionHi = glm::max(srcHi, dstHi) + Margin;
	return voxel::Region(unionLo, unionHi);
}

glm::ivec3 TransformBrush::transformPosition(const glm::ivec3 &pos) const {
	const glm::vec3 center = _snapshotCenter;
	glm::vec3 result(pos);

	switch (_transformMode) {
	case TransformMode::Move:
		result += glm::vec3(_moveOffset);
		break;

	case TransformMode::Shear: {
		// Shear transform: each slice along a perpendicular axis shifts proportionally
		// to its distance from the center - like pushing the top of a bread stack sideways.
		// X shear: each Y-layer shifts in X (top goes +X, bottom goes -X)
		// Y shear: each Z-layer shifts in Y (front goes +Y, back goes -Y)
		// Z shear: each X-layer shifts in Z (right goes +Z, left goes -Z)
		const glm::vec3 relative = result - center;
		const glm::vec3 halfSize = glm::vec3(_snapshotHelper.snapshotRegion().getDimensionsInVoxels()) * 0.5f;

		if (halfSize.y > 0.0f && _shearOffset.x != 0) {
			result.x += (relative.y / halfSize.y) * (float)_shearOffset.x;
		}
		if (halfSize.z > 0.0f && _shearOffset.y != 0) {
			result.y += (relative.z / halfSize.z) * (float)_shearOffset.y;
		}
		if (halfSize.x > 0.0f && _shearOffset.z != 0) {
			result.z += (relative.x / halfSize.x) * (float)_shearOffset.z;
		}
		break;
	}

	case TransformMode::Scale: {
		const glm::vec3 relative = result - center;
		result = center + relative * _scale;
		break;
	}

	case TransformMode::Rotate: {
		glm::vec3 relative = result - center;
		if (_rotationDegrees.x != 0.0f) {
			relative = glm::rotateX(relative, glm::radians(_rotationDegrees.x));
		}
		if (_rotationDegrees.y != 0.0f) {
			relative = glm::rotateY(relative, glm::radians(_rotationDegrees.y));
		}
		if (_rotationDegrees.z != 0.0f) {
			relative = glm::rotateZ(relative, glm::radians(_rotationDegrees.z));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return glm::ivec3(glm::round(result));
}

glm::vec3 TransformBrush::inverseTransformPosition(const glm::ivec3 &pos) const {
	const glm::vec3 center = _snapshotCenter;
	glm::vec3 result(pos);

	switch (_transformMode) {
	case TransformMode::Scale: {
		// Inverse of: result = center + relative * scale
		const glm::vec3 relative = result - center;
		result = center + relative / _scale;
		break;
	}

	case TransformMode::Rotate: {
		// Inverse rotation: apply in reverse order with negated angles
		glm::vec3 relative = result - center;
		if (_rotationDegrees.z != 0.0f) {
			relative = glm::rotateZ(relative, glm::radians(-_rotationDegrees.z));
		}
		if (_rotationDegrees.y != 0.0f) {
			relative = glm::rotateY(relative, glm::radians(-_rotationDegrees.y));
		}
		if (_rotationDegrees.x != 0.0f) {
			relative = glm::rotateX(relative, glm::radians(-_rotationDegrees.x));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return result;
}

void TransformBrush::eraseSnapshotPositions(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(EraseSnapshotPositions);

	// Iterate only the populated entries in the sparse snapshot instead of the
	// full bounding box - avoids hash lookups for empty positions.
	struct Eraser {
		ModifierVolumeWrapper *wrapper;
		SnapshotHelper *helper;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &) {
			constexpr voxel::Voxel air;
			const glm::ivec3 pos(x, y, z);
			helper->writeVoxel(*wrapper, pos, air);
			return true;
		}
	};
	Eraser eraser{&wrapper, &_snapshotHelper};
	_snapshotHelper.snapshot().copyTo(eraser);
}

void TransformBrush::applyInverseMapping(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(ApplyInverseMapping);
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Convert sparse snapshot to a temporary RawVolume as it is a cache-friendly flat array
	const voxel::Region &snapshotRegion = _snapshotHelper.snapshotRegion();
	voxel::RawVolume snapshotRaw(snapshotRegion);
	_snapshotHelper.snapshot().copyTo(snapshotRaw);

	const glm::ivec3 &srcLo = snapshotRegion.getLowerCorner();
	const glm::ivec3 &srcHi = snapshotRegion.getUpperCorner();
	glm::ivec3 dstLo(INT_MAX);
	glm::ivec3 dstHi(INT_MIN);

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const glm::ivec3 cornerPos((corner & 1) ? srcHi.x : srcLo.x, (corner & 2) ? srcHi.y : srcLo.y,
								   (corner & 4) ? srcHi.z : srcLo.z);
		const glm::ivec3 transformed = transformPosition(cornerPos);
		dstLo = glm::min(dstLo, transformed);
		dstHi = glm::max(dstHi, transformed);
	}

	// Clamp to volume bounds
	dstLo = glm::max(dstLo, volRegion.getLowerCorner());
	dstHi = glm::min(dstHi, volRegion.getUpperCorner());

	const int zSlices = dstHi.z - dstLo.z + 1;
	if (zSlices <= 0) {
		return;
	}

	// Collect transformed voxels in parallel, then write sequentially.
	// Writing must be sequential because writeVoxel updates history (hash map).
	// One result array per Z slice - no contention between threads
	core::DynamicArray<voxel::DynamicVoxelArray> sliceResults;
	sliceResults.resize(zSlices);

	app::for_parallel(dstLo.z, dstHi.z + 1, [this, &snapshotRaw, &sliceResults, dstLo, dstHi](int startZ, int endZ) {
		voxel::RawVolume::Sampler sampler(snapshotRaw);
		for (int dz = startZ; dz < endZ; ++dz) {
			const int sliceIdx = dz - dstLo.z;
			voxel::DynamicVoxelArray &results = sliceResults[sliceIdx];
			results.reserve((dstHi.x - dstLo.x + 1) * (dstHi.y - dstLo.y + 1));
			for (int dy = dstLo.y; dy <= dstHi.y; ++dy) {
				for (int dx = dstLo.x; dx <= dstHi.x; ++dx) {
					const glm::ivec3 dstPos(dx, dy, dz);
					const glm::vec3 srcPos = inverseTransformPosition(dstPos);
					const voxel::Voxel source = voxel::sampleVoxel(sampler, _voxelSampling, srcPos);
					if (!voxel::isAir(source.getMaterial())) {
						results.push_back({dstPos, source});
					}
				}
			}
		}
	});

	// Write results sequentially
	for (const auto &results : sliceResults) {
		for (const auto &tv : results) {
			_snapshotHelper.writeVoxel(wrapper, tv.pos, tv.voxel);
		}
	}
}

void TransformBrush::applyTransform(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	core_trace_scoped(ApplyTransform);

	// Erase all original selected voxels (saving to history)
	eraseSnapshotPositions(wrapper);

	if (_transformMode == TransformMode::Scale || _transformMode == TransformMode::Rotate) {
		// Inverse mapping with parallelized computation for Scale and Rotate.
		// Scale needs inverse mapping to fill gaps on scale-up.
		// Rotate needs it because forward mapping leaves holes at non-90-degree angles.
		applyInverseMapping(wrapper);
	} else {
		// Forward mapping: Move and Shear produce 1:1 mappings without gaps
		voxelutil::visitVolume(
			_snapshotHelper.snapshot(), _snapshotHelper.snapshotRegion(),
			[&](int x, int y, int z, const voxel::Voxel &v) {
				const glm::ivec3 newPos = transformPosition(glm::ivec3(x, y, z));
				_snapshotHelper.writeVoxel(wrapper, newPos, v);
			},
			voxelutil::VisitSolid());
	}
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
	applyTransform(wrapper, ctx);
	markDirty();
}

bool TransformBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _snapshotHelper.hasSnapshot() || ctx.targetVolumeRegion.isValid();
}

void TransformBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.snap = (float)ctx.gridResolution;
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
		state.snap = 15.0f; // 15 degree snap for rotation
		break;
	}
	case TransformMode::Scale:
		state.operations = BrushGizmo_Scale;
		state.matrix = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), _scale);
		state.snap = 0.1f;
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
