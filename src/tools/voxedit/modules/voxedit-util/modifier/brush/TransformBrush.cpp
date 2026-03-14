/**
 * @file
 */

#include "TransformBrush.h"
#include "app/Async.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/VolumeSampler.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>
#include <glm/common.hpp>

namespace voxedit {

void TransformBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
}

void TransformBrush::onActivated() {
	reset();
	_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
}

bool TransformBrush::onDeactivated() {
	_sceneModifiedFlags = SceneModifiedFlags::All;
	return _hasSnapshot;
}

void TransformBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_snapshotCenter = glm::vec3(0.0f);
	_capturedVolumeLower = glm::ivec3(0);
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
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
	return _active || _hasSnapshot;
}

void TransformBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_hasSnapshot && volume != nullptr) {
		captureSnapshot(volume, ctx.targetVolumeRegion);
	} else if (_hasSnapshot) {
		// Detect volume region shift (e.g. node was moved) and adjust cached data
		const glm::ivec3 delta = ctx.targetVolumeRegion.getLowerCorner() - _capturedVolumeLower;
		if (delta != glm::ivec3(0)) {
			adjustSnapshotForRegionShift(delta);
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

void TransformBrush::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	core_trace_scoped(CaptureSnapshot);
	_snapshot.clear();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());

	const glm::ivec3 &regionLo = volRegion.getLowerCorner();
	const glm::ivec3 &regionHi = volRegion.getUpperCorner();
	// TODO: PERF: use visitor
	for (int z = regionLo.z; z <= regionHi.z; ++z) {
		for (int y = regionLo.y; y <= regionHi.y; ++y) {
			for (int x = regionLo.x; x <= regionHi.x; ++x) {
				const voxel::Voxel &currentVoxel = volume->voxel(x, y, z);
				if (voxel::isAir(currentVoxel.getMaterial())) {
					continue;
				}
				if (!(currentVoxel.getFlags() & voxel::FlagOutline)) {
					continue;
				}

				const glm::ivec3 pos(x, y, z);
				_snapshot.setVoxel(pos, currentVoxel);
				selLo = glm::min(selLo, pos);
				selHi = glm::max(selHi, pos);
			}
		}
	}

	if (_snapshot.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = voxel::Region(selLo, selHi);
	_snapshotCenter = glm::vec3(selLo + selHi) * 0.5f;
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
}

void TransformBrush::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
	core_trace_scoped(AdjustSnapshotForRegionShift);
	// Collect and re-insert with shifted positions
	voxel::DynamicVoxelArray entries;
	_snapshot.copyTo(entries, _snapshotRegion);
	_snapshot.clear();
	for (const voxel::VoxelPosition &e : entries) {
		_snapshot.setVoxel(delta + e.pos, e.voxel);
	}

	_snapshotRegion.shift(delta.x, delta.y, delta.z);
	_snapshotCenter += glm::vec3(delta);
	_capturedVolumeLower += delta;

	// Shift history positions: collect, clear, reinsert shifted
	struct EntryCollector {
		voxel::DynamicVoxelArray *entries;
		glm::ivec3 delta;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			entries->push_back({glm::ivec3(x + delta.x, y + delta.y, z + delta.z), voxel});
			return true;
		}
	};
	voxel::DynamicVoxelArray historyEntries;
	historyEntries.reserve(_history.size());
	EntryCollector collector{&historyEntries, delta};
	_history.copyTo(collector);
	_history.clear();
	for (const voxel::VoxelPosition &e : historyEntries) {
		_history.setVoxel(e.pos, e.voxel);
	}
}

voxel::Region TransformBrush::computeTransformedRegion() const {
	core_trace_scoped(ComputeTransformedRegion);
	if (!_hasSnapshot || _snapshot.empty()) {
		return voxel::Region::InvalidRegion;
	}
	const glm::ivec3 &srcLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &srcHi = _snapshotRegion.getUpperCorner();
	glm::ivec3 dstLo = srcLo;
	glm::ivec3 dstHi = srcHi;

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const glm::ivec3 cornerPos(
			(corner & 1) ? srcHi.x : srcLo.x,
			(corner & 2) ? srcHi.y : srcLo.y,
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
		const glm::vec3 halfSize = glm::vec3(_snapshotRegion.getDimensionsInVoxels()) * 0.5f;

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

void TransformBrush::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	if (_history.hasVoxel(pos)) {
		return;
	}
	_history.setVoxel(pos, vol->voxel(pos));
}

void TransformBrush::writeVoxel(ModifierVolumeWrapper &wrapper,
								const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *volume = wrapper.volume();
	if (!volume->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(volume, pos);
	if (volume->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

void TransformBrush::eraseSnapshotPositions(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(EraseSnapshotPositions);
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	const voxel::Voxel air;

	// Iterate only the populated entries in the sparse snapshot instead of the
	// full bounding box - avoids hash lookups for empty positions.
	struct Eraser {
		voxel::RawVolume *vol;
		ModifierVolumeWrapper *wrapper;
		TransformBrush *brush;
		const voxel::Region *volRegion;
		voxel::Voxel air;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &) {
			const glm::ivec3 pos(x, y, z);
			if (volRegion->containsPoint(pos)) {
				brush->writeVoxel(*wrapper, pos, air);
			}
			return true;
		}
	};
	Eraser eraser{vol, &wrapper, this, &volRegion, air};
	_snapshot.copyTo(eraser);
}

void TransformBrush::applyInverseMapping(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(ApplyInverseMapping);
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Convert sparse snapshot to a temporary RawVolume for fast lock-free reads.
	// ConcurrentSparseVolume::Sampler acquires a mutex per voxel lookup which causes massive
	// contention under parallel access. RawVolume is a flat array - cache-friendly
	// and requires no locks for concurrent reads.
	voxel::RawVolume snapshotRaw(_snapshotRegion);
	_snapshot.copyTo(snapshotRaw);

	const glm::ivec3 &srcLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &srcHi = _snapshotRegion.getUpperCorner();
	glm::ivec3 dstLo(INT_MAX);
	glm::ivec3 dstHi(INT_MIN);

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const glm::ivec3 cornerPos(
			(corner & 1) ? srcHi.x : srcLo.x,
			(corner & 2) ? srcHi.y : srcLo.y,
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
			writeVoxel(wrapper, tv.pos, tv.voxel);
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
		voxelutil::visitVolume(_snapshot, _snapshotRegion, [&](int x, int y, int z, const voxel::Voxel &v) {
			const glm::ivec3 newPos = transformPosition(glm::ivec3(x, y, z));
			writeVoxel(wrapper, newPos, v);
		}, voxelutil::VisitSolid());
	}
}

void TransformBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							  const voxel::Region &) {
	core_trace_scoped(Generate);
	if (!_hasSnapshot) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();
	_lastVolume = vol;

	// Restore previously transformed state before re-applying
	struct HistoryRestorer {
		voxel::RawVolume *vol;
		ModifierVolumeWrapper *wrapper;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			if (vol->setVoxel(x, y, z, voxel)) {
				wrapper->addToDirtyRegion(glm::ivec3(x, y, z));
			}
			return true;
		}
	};
	HistoryRestorer restorer{vol, &wrapper};
	_history.copyTo(restorer);
	_history.clear();

	// Apply the current transform from the original snapshot
	applyTransform(wrapper, ctx);
	markDirty();
}

bool TransformBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _hasSnapshot || ctx.targetVolumeRegion.isValid();
}

void TransformBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.snap = (float)ctx.gridResolution;
	state.localMode = true;

	const glm::vec3 center = _hasSnapshot ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

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

bool TransformBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
									 const glm::mat4 &deltaMatrix, uint32_t operation) {
	core_trace_scoped(ApplyBrushGizmo);
	const glm::vec3 center = _hasSnapshot ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

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
