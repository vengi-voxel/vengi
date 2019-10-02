/**
 * @file
 */

#pragma once

/**
 * @brief The skeleton attributes reflect the model values that are needed to
 * assemble the final mesh. This is mostly about offsets and positioning.
 */
struct SkeletonAttribute {
	float scaler = 1.0f;
	float toolRight = 6.0f;
	float toolForward = -6.1f;
	float toolScale = 1.0f;
	float neckRight = 0.0f;
	float neckForward = 0.0f;
	float neckHeight = 0.0f;
	float headScale = 1.0f;
	float handRight = -7.5f;
	float handForward = 0.0f;
	float shoulderRight = -5.0f;
	float shoulderForward = 0.0f;

	float runTimeFactor = 12.0f;
	float jumpTimeFactor = 14.0f;
	float idleTimeFactor = 0.3f;

	float shoulderScale = 1.1f;
	float hipOffset = 6.0f; // to shift the rotation point for the feet
	float origin = 0.0f;
	float footHeight = 3.0f;
	float invisibleLegHeight = 0.5f;
	float pantsHeight = 3.0f;
	float beltHeight = 2.0f;
	float chestHeight = 5.0f;
	float headHeight = 9.0f;
	float footRight = -3.2f;

	// not exposed but calculated
	float footY;
	float pantsY;
	float beltY;
	float chestY;
	float headY;
	float gliderY;

	SkeletonAttribute() {
		update();
	}

	/**
	 * @brief Updates some absolute values that depend on other scriptable values
	 * @note Make sure to call update() after you modified values
	 */
	void update() {
		footY = origin;
		pantsY = footY + footHeight + invisibleLegHeight;
		beltY = pantsY + pantsHeight;
		chestY = beltY + beltHeight;
		headY = chestY + chestHeight;
		gliderY = headY + headHeight;
	}
};
