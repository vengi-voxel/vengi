/**
 * @file
 */

#pragma once

#include "voxel/Face.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxelutil {

/**
 * @brief A structure containing the information about a picking operation
 */
struct PickResult {
	PickResult() {
	}

	/** Did the picking operation hit anything */
	bool didHit = false;
	/** Indicates whether @c firstPosition is valid */
	bool firstValidPosition = false;
	/** this might be false if the raycast started in a solid voxel */
	bool validPreviousPosition = false;
	bool firstInvalidPosition = false;

	/** The location of the solid voxel it hit */
	math::Vec3i hitVoxel {0};

	/** The location of the step before we end the trace - see @a validPreviousPosition */
	math::Vec3i previousPosition {0};
	/** The location where the trace entered the valid volume region */
	math::Vec3i firstPosition {0};

	math::Vec3f direction {0.0f};

	voxel::FaceNames hitFace = voxel::FaceNames::Max;
};

}
