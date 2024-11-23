#pragma once
#include <iostream>
#include <stack>
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
namespace BenVoxel {
class Branch;
struct Position {
	const std::uint16_t x;
	const std::uint16_t y;
	const std::uint16_t z;
	Position(std::uint16_t x, std::uint16_t y, std::uint16_t z);
};
class Node {
protected:
	std::uint8_t octant;
	Branch *parent; // Not owned, nullptr indicates root Branch
	static const std::uint8_t BRANCH_REGULAR = 0b00000000, BRANCH_COLLAPSED = 0b01000000, LEAF_2BYTE = 0b10000000,
							  LEAF_8BYTE = 0b11000000, TYPE_MASK = 0b11000000;

public:
	Node(Branch *parent, std::uint8_t header);
	Node(Branch *parent, std::istream &in);
	virtual ~Node() = default;
	Node(const Node &) = delete;
	Node &operator=(const Node &) = delete;
	virtual void write(std::ostream &out) const = 0;
	std::uint8_t getOctant() const;
	Branch *getParent() const;
	std::uint8_t depth() const;
	Position position() const;
	static std::uint8_t readByte(std::istream &in, const char *errorMessage = "Failed to read byte from input stream.");
	static std::uint8_t peekByte(std::istream &in,
								 const char *errorMessage = "Failed to peek at byte from input stream.");
};
} // namespace BenVoxel
