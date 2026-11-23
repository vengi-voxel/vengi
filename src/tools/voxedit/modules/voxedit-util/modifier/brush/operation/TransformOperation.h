/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/brush/SnapshotHelper.h"
#include "core/collection/Buffer.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxedit {

class ModifierVolumeWrapper;

enum class TransformMode : uint8_t {
	Move,
	Shear,
	Scale,
	Rotate,

	Max
};

/** Accepted values are shared by panel, handles, and headless requests. */
struct TransformParameters {
	TransformMode mode = TransformMode::Move;
	voxel::VoxelSampling sampling = voxel::VoxelSampling::Nearest;
	math::Vec3i translation{0};
	math::Vec3i shear{0};
	math::Vec3f scale{1.0f};
	math::Vec3f rotationDegrees{0.0f};

	bool operator==(const TransformParameters &other) const {
		return mode == other.mode && sampling == other.sampling && translation == other.translation &&
			   shear == other.shear && scale == other.scale && rotationDegrees == other.rotationDegrees;
	}
};

/** Per-target evaluator. It has no scene, UI, input, or undo dependency. */
class TransformOperation {
protected:
	TransformParameters _parameters;

	SnapshotHelper _snapshotHelper;
	// Center of selection (used as pivot for scale/rotate)
	math::Vec3f _snapshotCenter{0.0f};

	// Cached region for preview (union of snapshot + transformed bounding box)
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;
	// Previous frame's transformed destination region (for dirty region tracking)
	mutable voxel::Region _prevTransformedRegion = voxel::Region::InvalidRegion;

	// Transform parameters

	void applyTransform(ModifierVolumeWrapper &wrapper);
	void eraseSnapshotPositions(ModifierVolumeWrapper &wrapper);
	void applyInverseMapping(ModifierVolumeWrapper &wrapper);
	voxel::Region computeTransformedRegion() const;
	voxel::Region computeDestinationRegion() const;
	math::Vec3i transformPosition(const math::Vec3i &pos) const;
	math::Vec3f inverseTransformPosition(const math::Vec3i &pos) const;

public:
	static constexpr bool requiresSelection = true;
	using Parameters = TransformParameters;
	static bool sameOperation(const Parameters &a, const Parameters &b) {
		return a.mode == b.mode;
	}
	static constexpr int MaxMoveOffset = 250;
	static constexpr int MaxShearOffset = 250;
	const TransformParameters &parameters() const {
		return _parameters;
	}
	bool setParameters(const TransformParameters &parameters);
	void prepare(const voxel::RawVolume *volume, const voxel::Region &region);
	void evaluate(ModifierVolumeWrapper &wrapper);
	bool dirtyRegions(core::Buffer<voxel::Region> &regions) const;
	bool hasSnapshot() const {
		return _snapshotHelper.hasSnapshot();
	}
	size_t snapshotVoxelCount() const {
		return _snapshotHelper.snapshotVoxelCount();
	}
	const voxel::Region &snapshotRegion() const {
		return _snapshotHelper.snapshotRegion();
	}
};

} // namespace voxedit
