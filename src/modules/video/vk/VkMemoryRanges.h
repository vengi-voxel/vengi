/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/collection/DynamicArray.h"
#include <stdint.h>

namespace video {
namespace _priv {

struct VkMemoryRange {
	uint64_t offset = 0;
	uint64_t size = 0;
};

class VkMemoryRanges {
private:
	core::DynamicArray<VkMemoryRange, 4> _free;

public:
	void reset(uint64_t size) {
		_free.clear();
		_free.push_back({0, size});
	}

	bool allocate(uint64_t size, uint64_t alignment, VkMemoryRange &allocation) {
		for (size_t i = 0; i < _free.size(); ++i) {
			const VkMemoryRange range = _free[i];
			const uint64_t offset = (range.offset + alignment - 1) & ~(alignment - 1);
			const uint64_t end = range.offset + range.size;
			if (offset > end || size > end - offset) {
				continue;
			}
			const uint64_t next = offset + size;
			if (offset == range.offset) {
				if (next == end) {
					_free.erase(i);
				} else {
					_free[i] = {next, end - next};
				}
			} else {
				_free[i].size = offset - range.offset;
				if (next < end) {
					_free.insert(_free.begin() + i + 1, VkMemoryRange{next, end - next});
				}
			}
			allocation = {offset, size};
			return true;
		}
		return false;
	}

	void release(VkMemoryRange range) {
		size_t i = 0;
		while (i < _free.size() && _free[i].offset < range.offset) {
			++i;
		}
		if (i > 0) {
			VkMemoryRange &prev = _free[i - 1];
			core_assert(prev.offset + prev.size <= range.offset);
			if (prev.offset + prev.size == range.offset) {
				range.offset = prev.offset;
				range.size += prev.size;
				_free.erase(--i);
			}
		}
		if (i < _free.size()) {
			core_assert(range.offset + range.size <= _free[i].offset);
			if (range.offset + range.size == _free[i].offset) {
				_free[i] = {range.offset, range.size + _free[i].size};
				return;
			}
		}
		_free.insert(_free.begin() + i, range);
	}

	bool allFree(uint64_t size) const {
		return _free.size() == 1 && _free[0].offset == 0 && _free[0].size == size;
	}
};

} // namespace _priv
} // namespace video
