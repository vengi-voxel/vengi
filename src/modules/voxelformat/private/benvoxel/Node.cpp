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
namespace BenVoxel {
Position::Position(std::uint16_t x, std::uint16_t y, std::uint16_t z) : x(x), y(y), z(z) {
}
Node::Node(Branch *parent, std::istream &in) : parent(parent), octant(0) {
	octant = peekByte(in, "Failed to peek at node header byte from input stream.") & 0b111;
}
Node::Node(Branch *parent, std::uint8_t header) : octant(header & 0b111), parent(parent) {
}
std::uint8_t Node::getOctant() const {
	return octant & 0b111;
}
Branch *Node::getParent() const {
	return const_cast<Branch *>(parent);
}
std::uint8_t Node::depth() const {
	std::uint8_t d = 0;
	const Node *current = this;
	while (current->parent) {
		d++;
		current = current->parent;
	}
	return d;
}
Position Node::position() const {
	std::stack<Node *> stack = {};
	Node *node = const_cast<Node *>(this);
	while (node) {
		stack.push(node);
		node = dynamic_cast<Node *>(node->parent);
	}
	std::uint8_t count = 17 - static_cast<uint8_t>(stack.size());
	std::uint16_t x = 0, y = 0, z = 0;
	while (!stack.empty()) {
		node = stack.top();
		stack.pop();
		x = (x << 1) | node->getOctant() & 1;
		y = (y << 1) | (node->getOctant() >> 1) & 1;
		z = (z << 1) | (node->getOctant() >> 2) & 1;
	}
	x <<= count;
	y <<= count;
	z <<= count;
	return Position(x, y, z);
}
std::uint8_t Node::readByte(std::istream &in, const char *errorMessage) {
	int value = in.get();
	if (value < 0)
		throw std::runtime_error(errorMessage);
	return static_cast<std::uint8_t>(value);
}
std::uint8_t Node::peekByte(std::istream &in, const char *errorMessage) {
	int value = in.peek();
	if (value < 0)
		throw std::runtime_error(errorMessage);
	return static_cast<std::uint8_t>(value);
}
} // namespace BenVoxel
