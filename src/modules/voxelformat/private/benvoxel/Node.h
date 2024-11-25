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

#pragma once

#include "io/Stream.h"
#include <glm/vec3.hpp>

namespace BenVoxel {

class Branch;

using Position = glm::u16vec3;

enum class NodeType { Branch, Leaf };

class Node {
protected:
	uint8_t _octant;
	Branch *_parent; // Not owned, nullptr indicates root Branch
	NodeType _type;
	static const uint8_t BRANCH_REGULAR = 0b00000000, BRANCH_COLLAPSED = 0b01000000, LEAF_2BYTE = 0b10000000,
						 LEAF_8BYTE = 0b11000000, TYPE_MASK = 0b11000000;

public:
	Node(NodeType type, Branch *parent, uint8_t header);
	Node(NodeType type, Branch *parent, io::SeekableReadStream &in);
	virtual ~Node() = default;
	Node(const Node &) = delete;
	Node &operator=(const Node &) = delete;
	virtual void write(io::WriteStream &out) const = 0;
	NodeType type() const;
	uint8_t getOctant() const;
	Branch *getParent() const;
	uint8_t depth() const;
	Position position() const;
};

inline NodeType Node::type() const {
	return _type;
}

inline bool isBranch(const Node *node) {
	if (node == nullptr) {
		return false;
	}
	return node->type() == NodeType::Branch;
}

inline bool isLeaf(const Node *node) {
	if (node == nullptr) {
		return false;
	}
	return node->type() == NodeType::Leaf;
}

} // namespace BenVoxel
