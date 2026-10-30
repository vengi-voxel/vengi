/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/DynamicMap.h"
#include "math/Random.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxedit {

/**
 * Per-voxel stroke state for Blend/Blur: keep the original color and the strongest
 * falloff factor seen so far. Weak edge-first contact must not lock a voxel; when the
 * brush center later passes over it, upgrade from the original (never restack).
 */
struct PaintStrokeSample {
	voxel::Voxel original;
	float factor = 0.0f;
};

using PaintStrokeMap = core::DynamicMap<glm::ivec3, PaintStrokeSample, 1031, glm::hash<glm::ivec3>>;

/**
 * Per-target paint runtime for one edit. Owned by the session, never by a shared brush.
 */
struct PaintStrokeState {
	PaintStrokeMap blendStroke;
	math::Random random;
};

} // namespace voxedit
