/**
 * @file
 * @brief CPU-side draw lists produced by prepare and consumed by GPU submit.
 */

#pragma once

#include "core/collection/Buffer.h"

namespace voxelrender {

/**
 * Per-frame volume draw lists. Built off the submit path (prepare phase) so
 * main-thread submission only walks indices - no scene traversal or sorting.
 * Suitable as the source for multi-draw-indirect packing later.
 */
struct RenderFrame {
	core::Buffer<int> opaque;
	core::Buffer<int> transparent;
	bool anyVisible = false;

	void clear() {
		opaque.clear();
		transparent.clear();
		anyVisible = false;
	}

	void release() {
		opaque.release();
		transparent.release();
		anyVisible = false;
	}
};

} // namespace voxelrender
