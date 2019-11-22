/**
 * @file
 */

#pragma once

#include "animation/SkeletonAttribute.h"

namespace animation {

/**
 * @brief The skeleton attributes reflect the model values that are needed to
 * assemble the final mesh. This is mostly about offsets and positioning.
 */
struct CharacterSkeletonAttribute {
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

	CharacterSkeletonAttribute() {
		update();
	}

	/**
	 * @brief Updates some absolute values that depend on other scriptable values
	 * @note Make sure to call update() after you modified values
	 */
	bool update() {
		footY = origin;
		pantsY = footY + footHeight + invisibleLegHeight;
		beltY = pantsY + pantsHeight;
		chestY = beltY + beltHeight;
		headY = chestY + chestHeight;
		gliderY = headY + headHeight;
		return true;
	}
};

static const SkeletonAttributeMeta ChrSkeletonAttributeMetaArray[] = {
	{ "Scaler",             offsetof(struct CharacterSkeletonAttribute, scaler) },
	{ "ToolRight",          offsetof(struct CharacterSkeletonAttribute, toolRight) },
	{ "ToolForward",        offsetof(struct CharacterSkeletonAttribute, toolForward) },
	{ "ToolScale",          offsetof(struct CharacterSkeletonAttribute, toolScale) },
	{ "NeckRight",          offsetof(struct CharacterSkeletonAttribute, neckRight) },
	{ "NeckForward",        offsetof(struct CharacterSkeletonAttribute, neckForward) },
	{ "NeckHeight",         offsetof(struct CharacterSkeletonAttribute, neckHeight) },
	{ "HeadScale",          offsetof(struct CharacterSkeletonAttribute, headScale) },
	{ "HandRight",          offsetof(struct CharacterSkeletonAttribute, handRight) },
	{ "HandForward",        offsetof(struct CharacterSkeletonAttribute, handForward) },
	{ "ShoulderRight",      offsetof(struct CharacterSkeletonAttribute, shoulderRight) },
	{ "ShoulderForward",    offsetof(struct CharacterSkeletonAttribute, shoulderForward) },
	{ "RunTimeFactor",      offsetof(struct CharacterSkeletonAttribute, runTimeFactor) },
	{ "JumpTimeFactor",     offsetof(struct CharacterSkeletonAttribute, jumpTimeFactor) },
	{ "IdleTimeFactor",     offsetof(struct CharacterSkeletonAttribute, idleTimeFactor) },
	{ "ShoulderScale",      offsetof(struct CharacterSkeletonAttribute, shoulderScale) },
	{ "HipOffset",          offsetof(struct CharacterSkeletonAttribute, hipOffset) },
	{ "Origin",             offsetof(struct CharacterSkeletonAttribute, origin) },
	{ "FootHeight",         offsetof(struct CharacterSkeletonAttribute, footHeight) },
	{ "InvisibleLegHeight", offsetof(struct CharacterSkeletonAttribute, invisibleLegHeight) },
	{ "PantsHeight",        offsetof(struct CharacterSkeletonAttribute, pantsHeight) },
	{ "BeltHeight",         offsetof(struct CharacterSkeletonAttribute, beltHeight) },
	{ "ChestHeight",        offsetof(struct CharacterSkeletonAttribute, chestHeight) },
	{ "HeadHeight",         offsetof(struct CharacterSkeletonAttribute, headHeight) },
	{ "FootRight",          offsetof(struct CharacterSkeletonAttribute, footRight) },
	{ nullptr, 0u }
};

}
