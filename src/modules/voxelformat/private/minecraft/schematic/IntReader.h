/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"

namespace voxelformat {
namespace schematic {

class IntReader {
private:
	const core::Buffer<int8_t> *_blocks;
	int _index = 0;

public:
	IntReader(const core::Buffer<int8_t> *blocks) : _blocks(blocks) {
	}

	bool eos() const {
		if (_index >= (int)_blocks->size()) {
			return true;
		}
		return false;
	}

	int readInt32(int32_t &val) {
		if (eos()) {
			return -1;
		}
		int value = 0;
		for (int bitsRead = 0;; bitsRead += 7) {
			if (_index >= (int)_blocks->size()) {
				return -1;
			}
			uint8_t next = (*_blocks)[_index];
			_index++;
			value |= (next & 0x7F) << bitsRead;
			if (bitsRead > 7 * 5) {
				return -1;
			}
			if ((next & 0x80) == 0) {
				break;
			}
		}
		val = value;
		return 0;
	}
};

} // namespace schematic
} // namespace voxelformat
