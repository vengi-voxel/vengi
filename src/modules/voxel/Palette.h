/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Array.h"
#include <stdint.h>

namespace voxel {

struct Palette {
	core::Array<uint32_t, 256> _colors;
	core::Array<uint32_t, 256> _glowColors;
    core::String lua;
    size_t colorCount = 0;

	inline size_t size() const {
		return colorCount;
	}
};

}