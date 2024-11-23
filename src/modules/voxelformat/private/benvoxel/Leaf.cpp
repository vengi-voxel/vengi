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
namespace BenVoxel {
Leaf::Leaf(Branch *parent, std::uint8_t octant) : Node(parent, octant & 0b111), data{} {
}
Leaf::Leaf(Branch *parent, std::uint8_t octant, std::uint8_t color) : Node(parent, octant & 0b111), data{} {
	data.fill(color);
}
Leaf::Leaf(Branch *parent, std::istream &in) : Node(parent, in), data{} {
	std::uint8_t header = readByte(in, "Failed to read leaf header byte from input stream.");
	switch (header & TYPE_MASK) {
	case LEAF_2BYTE: {
		std::uint8_t foreground = readByte(in, "Failed to read foreground voxel from input stream."),
					 background = readByte(in, "Failed to read background voxel from input stream.");
		std::uint8_t where = (header >> 3) & 0b111;
		for (std::uint8_t i = 0; i < 8; i++)
			data[i] = (i == where) ? foreground : background;
		break;
	}
	case LEAF_8BYTE: {
		in.read(reinterpret_cast<char *>(data.data()), 8);
		break;
	}
	default:
		throw std::runtime_error("Invalid leaf node header type. Expected 10xxxxxx or 11xxxxxx");
	}
}
void Leaf::write(std::ostream &out) const {
	std::array<std::pair<std::uint8_t, std::uint8_t>, 8> occurrences;
	std::uint8_t uniqueCount = 0;
	for (std::uint8_t value : data) {
		std::array<std::pair<std::uint8_t, std::uint8_t>, 8>::iterator it =
			std::find_if(occurrences.begin(), occurrences.begin() + uniqueCount,
						 [value](const std::pair<std::uint8_t, std::uint8_t> &p) { return p.first == value; });
		if (it != occurrences.begin() + uniqueCount)
			it->second++;
		else if (uniqueCount < 8)
			occurrences[uniqueCount++] = std::pair<std::uint8_t, std::uint8_t>(value, 1);
	}
	// Sort by count (ascending), but only sort the portion we've used
	std::sort(occurrences.begin(), occurrences.begin() + uniqueCount,
			  [](const std::pair<std::uint8_t, std::uint8_t> &a, const std::pair<std::uint8_t, std::uint8_t> &b) {
				  return a.second < b.second;
			  });
	if (uniqueCount == 1) {					  // Single color - use 2-byte payload leaf with same color
		out.put(LEAF_2BYTE | octant & 0b111); // Header
		out.put(occurrences[0].first);		  // Color for both foreground and background
		out.put(occurrences[0].first);
	} else if (uniqueCount == 2 && occurrences[0].second == 1) { // Two colors with one unique - use 2-byte payload leaf
		std::uint8_t uniqueIndex = static_cast<std::uint8_t>(
			std::distance(data.begin(), std::find(data.begin(), data.end(), occurrences[0].first)));
		out.put(LEAF_2BYTE | (uniqueIndex & 0b111) << 3 | octant & 0b111); // Header
		out.put(occurrences[0].first);									   // Foreground (unique)
		out.put(occurrences[1].first);									   // Background (repeated)
	} else {															   // Multiple colors - use 8-byte payload leaf
		out.put(LEAF_8BYTE | octant & 0b111);							   // Header
		out.write(reinterpret_cast<const char *>(data.data()), 8);		   // Data
	}
}
std::uint8_t Leaf::operator[](std::uint8_t octant) const {
	return data[octant];
}
void Leaf::set(std::uint8_t octant, std::uint8_t index) {
	data[octant] = index;
	if (parent &&
		data ==
			std::array<std::uint8_t, 8>{}) // This checks for all zeroes and modern compilers won't allocate a new array
		parent->remove(octant);
}
} // namespace BenVoxel
