// MIT License
//
// Copyright (c) 2024 Ben McLean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Leaf.h"
#include "core/Algorithm.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/Pair.h"
#include "core/collection/Array.h"
#include "io/Stream.h"

namespace BenVoxel {

Leaf::Leaf(Branch *parent, uint8_t octant) : Node(NodeType::Leaf, parent, octant & 0b111) {
}

Leaf::Leaf(Branch *parent, uint8_t octant, uint8_t color) : Node(NodeType::Leaf, parent, octant & 0b111) {
	_data.fill(color);
}

Leaf::Leaf(Branch *parent, io::SeekableReadStream &in) : Node(NodeType::Leaf, parent, in), _data{} {
	uint8_t header;
	if (in.readUInt8(header) == -1) {
		Log::error("Failed to read leaf header byte from input stream.");
		return;
	}
	switch (header & TYPE_MASK) {
	case LEAF_2BYTE: {
		uint8_t foreground;
		uint8_t background;
		if (in.readUInt8(foreground) == -1) {
			Log::error("Failed to read foreground voxel from input stream.");
		}
		if (in.readUInt8(background) == -1) {
			Log::error("Failed to read background voxel from input stream.");
		}
		const uint8_t where = (header >> 3) & 0b111;
		for (uint8_t i = 0; i < 8; i++) {
			_data[i] = (i == where) ? foreground : background;
		}
		break;
	}
	case LEAF_8BYTE: {
		for (int i = 0; i < 8; ++i) {
			in.readUInt8(_data[i]);
		}
		break;
	}
	default:
		Log::error("Invalid leaf node header type. Expected 10xxxxxx or 11xxxxxx");
		break;
	}
}

void Leaf::write(io::WriteStream &out) const {
	core::Array<core::Pair<uint8_t, uint8_t>, 8> occurrences;
	uint8_t uniqueCount = 0;
	for (uint8_t value : _data) {
		auto it = core::find_if(occurrences.begin(), occurrences.begin() + uniqueCount,
								[value](const auto &p) { return p.first == value; });
		if (it != occurrences.begin() + uniqueCount) {
			it->second++;
		} else if (uniqueCount < 8) {
			occurrences[uniqueCount++] = {value, 1};
		}
	}
	// Sort by count (ascending), but only sort the portion we've used
	core::sort(occurrences.begin(), occurrences.begin() + uniqueCount,
			   [](const auto &a, const auto &b) { return a.second < b.second; });
	if (uniqueCount == 1) {								// Single color - use 2-byte payload leaf with same color
		out.writeUInt8(LEAF_2BYTE | (_octant & 0b111)); // Header
		out.writeUInt8(occurrences[0].first);			// Color for both foreground and background
		out.writeUInt8(occurrences[0].first);
	} else if (uniqueCount == 2 && occurrences[0].second == 1) { // Two colors with one unique - use 2-byte payload leaf
		uint8_t uniqueIndex = static_cast<uint8_t>(
			core::distance(_data.begin(), core::find(_data.begin(), _data.end(), occurrences[0].first)));
		out.writeUInt8(LEAF_2BYTE | (uniqueIndex & 0b111) << 3 | (_octant & 0b111)); // Header
		out.writeUInt8(occurrences[0].first);										 // Foreground (unique)
		out.writeUInt8(occurrences[1].first);										 // Background (repeated)
	} else {											// Multiple colors - use 8-byte payload leaf
		out.writeUInt8(LEAF_8BYTE | (_octant & 0b111)); // Header
		for (int i = 0; i < 8; ++i) {
			out.writeUInt8(_data[i]); // Data
		}
	}
}

uint8_t Leaf::operator[](uint8_t octant) const {
	core_assert_msg(octant < 8, "Octant index out of bounds.");
	return _data[octant];
}

static inline bool isAllZero(const core::Array<uint8_t, 8> &data) {
	for (uint8_t value : data) {
		if (value != 0) {
			return false;
		}
	}
	return true;
}

void Leaf::set(uint8_t octant, uint8_t index) {
	core_assert_msg(octant < 8, "Octant index out of bounds.");
	_data[octant] = index;
	if (_parent && isAllZero(_data)) {
		_parent->remove(octant);
	}
}

} // namespace BenVoxel
