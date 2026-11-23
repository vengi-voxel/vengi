/**
 * @file
 * @brief CPU-side draw lists produced by prepare and consumed by GPU submit.
 */

#pragma once

#include "core/collection/Buffer.h"
#include <stdint.h>
#include "math/Mat4f.h"

namespace voxelrender {

/**
 * Per-draw instance data uploaded to the draw-instance SSBO for MDI (std430).
 * Layout must match DrawInstance in shaders/_drawinstance.glsl.
 */
struct DrawInstanceData {
	math::Mat4f model{1.0f};
	int32_t gray = 0;
	int32_t locked = 0;
	float opacity = 1.0f;
	int32_t pad = 0;
};
static_assert(sizeof(DrawInstanceData) == 80, "DrawInstanceData must match GLSL std430 DrawInstance");

/**
 * Per-frame volume draw lists. Built off the submit path (prepare phase) so
 * main-thread submission only walks indices - no scene traversal or sorting.
 * Suitable as the source for multi-draw-indirect packing.
 */
struct RenderFrame {
	core::Buffer<int> opaque;
	core::Buffer<int> transparent;
	bool anyVisible = false;
	bool sceneHasGlow = false;

	void clear() {
		opaque.clear();
		transparent.clear();
		anyVisible = false;
		sceneHasGlow = false;
	}

	void release() {
		opaque.release();
		transparent.release();
		anyVisible = false;
		sceneHasGlow = false;
	}
};

} // namespace voxelrender
