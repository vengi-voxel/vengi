/**
 * @file
 */

#include "TransformOperation.h"
#include "core/TraceScoped.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <limits.h>
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"


namespace voxedit {

bool TransformOperation::setParameters(const TransformParameters &parameters) {
	if (parameters.mode < TransformMode::Move || parameters.mode >= TransformMode::Max ||
		parameters.sampling >= voxel::VoxelSampling::Max) {
		return false;
	}
	for (int i = 0; i < 3; ++i) {
		if (!math::isfinite(parameters.scale[i]) || !math::isfinite(parameters.rotationDegrees[i])) {
			return false;
		}
	}
	_parameters = parameters;
	_parameters.translation = math::clamp(parameters.translation, math::Vec3i(-MaxMoveOffset), math::Vec3i(MaxMoveOffset));
	_parameters.shear = math::clamp(parameters.shear, math::Vec3i(-MaxShearOffset), math::Vec3i(MaxShearOffset));
	_parameters.scale = math::max(parameters.scale, math::Vec3f(0.01f));
	return true;
}

void TransformOperation::prepare(const voxel::RawVolume *volume, const voxel::Region &region) {
	if (!_snapshotHelper.hasSnapshot() && volume != nullptr) {
		_snapshotHelper.captureSnapshot(volume, region);
		if (_snapshotHelper.hasSnapshot()) {
			const voxel::Region &sr = _snapshotHelper.snapshotRegion();
			_snapshotCenter = math::Vec3f(sr.getLowerCorner() + sr.getUpperCorner()) * 0.5f;
		}
	} else if (_snapshotHelper.hasSnapshot()) {
		// Detect volume region shift (e.g. node was moved) and adjust cached data
		const math::Vec3i delta = region.getLowerCorner() - _snapshotHelper.capturedVolumeLower();
		if (delta != math::Vec3i(0)) {
			_snapshotHelper.adjustForRegionShift(delta);
			_snapshotCenter += math::Vec3f(delta);
		}
	}
	_cachedRegion = computeTransformedRegion();
	_cachedRegionValid = _cachedRegion.isValid();
}

voxel::Region TransformOperation::computeTransformedRegion() const {
	core_trace_scoped(ComputeTransformedRegion);
	if (!_snapshotHelper.hasSnapshot() || _snapshotHelper.snapshotVoxelCount() == 0) {
		return voxel::Region::InvalidRegion;
	}
	const math::Vec3i &srcLo = _snapshotHelper.snapshotRegion().getLowerCorner();
	const math::Vec3i &srcHi = _snapshotHelper.snapshotRegion().getUpperCorner();
	math::Vec3i dstLo = srcLo;
	math::Vec3i dstHi = srcHi;

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const math::Vec3i cornerPos((corner & 1) ? srcHi.x : srcLo.x, (corner & 2) ? srcHi.y : srcLo.y,
								   (corner & 4) ? srcHi.z : srcLo.z);
		const math::Vec3i transformed = transformPosition(cornerPos);
		dstLo = math::min(dstLo, transformed);
		dstHi = math::max(dstHi, transformed);
	}

	// Union of original and transformed regions with a small margin for rounding
	static constexpr int Margin = 2;
	const math::Vec3i unionLo = math::min(srcLo, dstLo) - Margin;
	const math::Vec3i unionHi = math::max(srcHi, dstHi) + Margin;
	return voxel::Region(unionLo, unionHi);
}

voxel::Region TransformOperation::computeDestinationRegion() const {
	if (!_snapshotHelper.hasSnapshot() || _snapshotHelper.snapshotVoxelCount() == 0) {
		return voxel::Region::InvalidRegion;
	}
	const math::Vec3i &srcLo = _snapshotHelper.snapshotRegion().getLowerCorner();
	const math::Vec3i &srcHi = _snapshotHelper.snapshotRegion().getUpperCorner();
	math::Vec3i dstLo(INT_MAX);
	math::Vec3i dstHi(INT_MIN);

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const math::Vec3i cornerPos((corner & 1) ? srcHi.x : srcLo.x, (corner & 2) ? srcHi.y : srcLo.y,
								   (corner & 4) ? srcHi.z : srcLo.z);
		const math::Vec3i transformed = transformPosition(cornerPos);
		dstLo = math::min(dstLo, transformed);
		dstHi = math::max(dstHi, transformed);
	}
	static constexpr int Margin = 2;
	return voxel::Region(dstLo - Margin, dstHi + Margin);
}

bool TransformOperation::dirtyRegions(core::Buffer<voxel::Region> &regions) const {
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

math::Vec3i TransformOperation::transformPosition(const math::Vec3i &pos) const {
	const math::Vec3f center = _snapshotCenter;
	math::Vec3f result(pos);

	switch (_parameters.mode) {
	case TransformMode::Move:
		result += math::Vec3f(_parameters.translation);
		break;

	case TransformMode::Shear: {
		// Shear transform: each slice along a perpendicular axis shifts proportionally
		// to its distance from the center - like pushing the top of a bread stack sideways.
		// X shear: each Y-layer shifts in X (top goes +X, bottom goes -X)
		// Y shear: each Z-layer shifts in Y (front goes +Y, back goes -Y)
		// Z shear: each X-layer shifts in Z (right goes +Z, left goes -Z)
		const math::Vec3f relative = result - center;
		const math::Vec3f halfSize = math::Vec3f(_snapshotHelper.snapshotRegion().getDimensionsInVoxels()) * 0.5f;

		if (halfSize.y > 0.0f && _parameters.shear.x != 0) {
			result.x += (relative.y / halfSize.y) * (float)_parameters.shear.x;
		}
		if (halfSize.z > 0.0f && _parameters.shear.y != 0) {
			result.y += (relative.z / halfSize.z) * (float)_parameters.shear.y;
		}
		if (halfSize.x > 0.0f && _parameters.shear.z != 0) {
			result.z += (relative.x / halfSize.x) * (float)_parameters.shear.z;
		}
		break;
	}

	case TransformMode::Scale: {
		const math::Vec3f relative = result - center;
		result = center + relative * _parameters.scale;
		break;
	}

	case TransformMode::Rotate: {
		math::Vec3f relative = result - center;
		if (_parameters.rotationDegrees.x != 0.0f) {
			relative = math::rotateX(relative, math::radians(_parameters.rotationDegrees.x));
		}
		if (_parameters.rotationDegrees.y != 0.0f) {
			relative = math::rotateY(relative, math::radians(_parameters.rotationDegrees.y));
		}
		if (_parameters.rotationDegrees.z != 0.0f) {
			relative = math::rotateZ(relative, math::radians(_parameters.rotationDegrees.z));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return math::Vec3i(math::round(result));
}

math::Vec3f TransformOperation::inverseTransformPosition(const math::Vec3i &pos) const {
	const math::Vec3f center = _snapshotCenter;
	math::Vec3f result(pos);

	switch (_parameters.mode) {
	case TransformMode::Scale: {
		// Inverse of: result = center + relative * scale
		const math::Vec3f relative = result - center;
		result = center + relative / _parameters.scale;
		break;
	}

	case TransformMode::Rotate: {
		// Inverse rotation: apply in reverse order with negated angles
		math::Vec3f relative = result - center;
		if (_parameters.rotationDegrees.z != 0.0f) {
			relative = math::rotateZ(relative, math::radians(-_parameters.rotationDegrees.z));
		}
		if (_parameters.rotationDegrees.y != 0.0f) {
			relative = math::rotateY(relative, math::radians(-_parameters.rotationDegrees.y));
		}
		if (_parameters.rotationDegrees.x != 0.0f) {
			relative = math::rotateX(relative, math::radians(-_parameters.rotationDegrees.x));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return result;
}

void TransformOperation::eraseSnapshotPositions(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(EraseSnapshotPositions);

	// Iterate only the populated entries in the sparse snapshot instead of the
	// full bounding box - avoids hash lookups for empty positions.
	struct Eraser {
		ModifierVolumeWrapper *wrapper;
		SnapshotHelper *helper;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &) {
			constexpr voxel::Voxel air;
			const math::Vec3i pos(x, y, z);
			helper->writeVoxel(*wrapper, pos, air);
			return true;
		}
	};
	Eraser eraser{&wrapper, &_snapshotHelper};
	_snapshotHelper.snapshot().copyTo(eraser);
}

void TransformOperation::applyInverseMapping(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(ApplyInverseMapping);
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Convert sparse snapshot to a temporary RawVolume as it is a cache-friendly flat array
	const voxel::Region &snapshotRegion = _snapshotHelper.snapshotRegion();
	voxel::RawVolume snapshotRaw(snapshotRegion);
	_snapshotHelper.snapshot().copyTo(snapshotRaw);

	const math::Vec3i &srcLo = snapshotRegion.getLowerCorner();
	const math::Vec3i &srcHi = snapshotRegion.getUpperCorner();
	math::Vec3i dstLo(INT_MAX);
	math::Vec3i dstHi(INT_MIN);

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const math::Vec3i cornerPos((corner & 1) ? srcHi.x : srcLo.x, (corner & 2) ? srcHi.y : srcLo.y,
								   (corner & 4) ? srcHi.z : srcLo.z);
		const math::Vec3i transformed = transformPosition(cornerPos);
		dstLo = math::min(dstLo, transformed);
		dstHi = math::max(dstHi, transformed);
	}

	// Clamp to volume bounds
	dstLo = math::max(dstLo, volRegion.getLowerCorner());
	dstHi = math::min(dstHi, volRegion.getUpperCorner());

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
					const math::Vec3i dstPos(dx, dy, dz);
					const math::Vec3f srcPos = inverseTransformPosition(dstPos);
					const voxel::Voxel source = voxel::sampleVoxel(sampler, _parameters.sampling, srcPos);
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

void TransformOperation::applyTransform(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(ApplyTransform);

	// Erase all original selected voxels (saving to history)
	eraseSnapshotPositions(wrapper);

	if (_parameters.mode == TransformMode::Scale || _parameters.mode == TransformMode::Rotate) {
		// Inverse mapping with parallelized computation for Scale and Rotate.
		// Scale needs inverse mapping to fill gaps on scale-up.
		// Rotate needs it because forward mapping leaves holes at non-90-degree angles.
		applyInverseMapping(wrapper);
	} else {
		// Forward mapping: Move and Shear produce 1:1 mappings without gaps
		voxelutil::visitVolume(
			_snapshotHelper.snapshot(), _snapshotHelper.snapshotRegion(),
			[&](int x, int y, int z, const voxel::Voxel &v) {
				const math::Vec3i newPos = transformPosition(math::Vec3i(x, y, z));
				_snapshotHelper.writeVoxel(wrapper, newPos, v);
			},
			voxelutil::VisitSolid());
	}
}

void TransformOperation::evaluate(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(Generate);
	if (!_snapshotHelper.hasSnapshot()) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();

	// Restore previously transformed state before re-applying
	_snapshotHelper.restoreHistory(vol, wrapper);

	// Apply the current transform from the original snapshot
	applyTransform(wrapper);
}

} // namespace voxedit
