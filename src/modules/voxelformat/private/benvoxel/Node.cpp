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

#include "Node.h"
#include "Branch.h"
#include "core/Log.h"
#include "core/collection/DynamicStack.h"
#include "io/Stream.h"

namespace BenVoxel {

Node::Node(NodeType type, Branch *parent, io::SeekableReadStream &in) : _parent(parent), _type(type) {
	if (in.peekUInt8(_octant) == -1) {
		Log::error("Failed to peek at node header byte from input stream.");
		_octant = 0;
	}
	_octant &= 0b111;
}

Node::Node(NodeType type, Branch *parent, uint8_t header) : _octant(header & 0b111), _parent(parent), _type(type) {
}

uint8_t Node::getOctant() const {
	return _octant & 0b111;
}

Branch *Node::getParent() const {
	return _parent;
}

uint8_t Node::depth() const {
	uint8_t d = 0;
	const Node *current = this;
	while (current->_parent) {
		d++;
		current = current->_parent;
	}
	return d;
}

Position Node::position() const {
	core::DynamicStack<const Node *> stack = {};
	const Node *node = this;
	while (node) {
		stack.push(node);
		node = node->_parent;
	}
	uint8_t count = 17 - (uint8_t)stack.size();
	uint16_t x = 0, y = 0, z = 0;
	while (!stack.empty()) {
		node = stack.pop();
		x = (x << 1) | (node->getOctant() & 1);
		y = (y << 1) | ((node->getOctant() >> 1) & 1);
		z = (z << 1) | ((node->getOctant() >> 2) & 1);
	}
	x <<= count;
	y <<= count;
	z <<= count;
	return Position(x, y, z);
}

} // namespace BenVoxel
