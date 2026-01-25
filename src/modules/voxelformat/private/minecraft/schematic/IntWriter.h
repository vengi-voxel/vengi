/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"

namespace voxelformat {
namespace schematic {

class IntWriter {
private:
	static constexpr const uint32_t SEGMENT_BITS = 0x7F;
	static constexpr const uint32_t CONTINUE_BIT = 0x80;
	core::Buffer<int8_t> &_blocks;

public:
	IntWriter(core::Buffer<int8_t> &blocks) : _blocks(blocks) {
	}

	void writeInt32(int32_t value) {
		while (true) {
			if ((value & ~SEGMENT_BITS) == 0) {
				_blocks.push_back((int8_t)value);
				return;
			}

			_blocks.push_back((int8_t)((value & SEGMENT_BITS) | CONTINUE_BIT));

			value >>= 7;
		}
	};
};

} // namespace schematic
} // namespace voxelformat
