/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "voxel/Voxel.h"
#include "Raycast.h"
#include "voxel/Face.h"

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
	glm::ivec3 hitVoxel {0};

	/** The location of the step before we end the trace - see @a validPreviousPosition */
	glm::ivec3 previousPosition {0};
	/** The location where the trace entered the valid volume region */
	glm::ivec3 firstPosition {0};

	glm::vec3 direction {0.0f};

	voxel::FaceNames hitFace = voxel::FaceNames::Max;
};

}
