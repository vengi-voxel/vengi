/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/Buffer.h"
#include "voxel/Voxel.h"
#include "SnapshotHelper.h"

namespace voxedit {

class ModifierVolumeWrapper;

enum class TransformMode : uint8_t {
	Move,
	Shear,
	Scale,
	Rotate,

	Max
};

/** Geometry and captured voxel data used by TransformBrush. */
class TransformOperation {
protected:
	TransformMode _transformMode = TransformMode::Move;
	voxel::VoxelSampling _voxelSampling = voxel::VoxelSampling::Nearest;

	SnapshotHelper _snapshotHelper;
	// Center of selection (used as pivot for scale/rotate)
	glm::vec3 _snapshotCenter{0.0f};

	// Cached region for preview (union of snapshot + transformed bounding box)
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;
	// Previous frame's transformed destination region (for dirty region tracking)
	mutable voxel::Region _prevTransformedRegion = voxel::Region::InvalidRegion;

	// Transform parameters
	glm::ivec3 _moveOffset{0};
	glm::ivec3 _shearOffset{0};
	glm::vec3 _scale{1.0f, 1.0f, 1.0f};
	glm::vec3 _rotationDegrees{0.0f, 0.0f, 0.0f};

	void applyTransform(ModifierVolumeWrapper &wrapper);
	void eraseSnapshotPositions(ModifierVolumeWrapper &wrapper);
	void applyInverseMapping(ModifierVolumeWrapper &wrapper);
	voxel::Region computeTransformedRegion() const;
	voxel::Region computeDestinationRegion() const;
	glm::ivec3 transformPosition(const glm::ivec3 &pos) const;
	glm::vec3 inverseTransformPosition(const glm::ivec3 &pos) const;
};

} // namespace voxedit
