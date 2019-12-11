/**
 * @file
 */

#pragma once

#include "animation/SkeletonAttribute.h"

namespace animation {

/**
 * @brief The skeleton attributes reflect the model values that are needed to
 * assemble the final mesh. This is mostly about offsets and positioning.
 *
 * @note This must be float values
 * @ingroup Animation
 */
struct BirdSkeletonAttribute : public SkeletonAttribute {
	BirdSkeletonAttribute();

	float scaler = 0.5f;
	float headScale = 1.0f;
	float origin = 0.0f;
	float footHeight = 3.0f;
	float footRight = -3.2f;
	float wingHeight = 8.0f;
	float wingRight = -4.2f;
	float invisibleLegHeight = 0.5f;
	float headHeight = 9.0f;
	float bodyHeight = 3.0f;

	// not exposed but calculated
	float footY = 0.0f;
	float bodyY = 0.0f;
	float headY = 0.0f;

	/**
	 * @brief Updates some absolute values that depend on other scriptable values
	 * @note Make sure to call init() after you modified the values
	 */
	bool init() {
		footY = origin;
		bodyY = footY + footHeight;
		headY = bodyY + bodyHeight;
		// TODO: perform sanity checks
		return true;
	}
};

}
