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

#include "Branch.h"
#include "Leaf.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "io/Stream.h"

namespace BenVoxel {

Branch::Branch() : Node(NodeType::Branch, nullptr, 0) {
}

Branch::Branch(Branch *parent, uint8_t octant) : Node(NodeType::Branch, parent, octant & 0b111) {
}

Branch::Branch(Branch *parent, uint8_t octant, uint8_t color) : Node(NodeType::Branch, parent, octant & 0b111) {
	expandCollapsed(color);
}

Branch::Branch(Branch *parent, io::SeekableReadStream &in) : Node(NodeType::Branch, parent, in) {
	uint8_t header = 0;
	if (in.readUInt8(header) == -1) {
		Log::error("Failed to read branch header byte from input stream.");
		return;
	}
	switch (header & TYPE_MASK) {
	case BRANCH_REGULAR: {
		uint8_t count = ((header >> 3) & 0b111) + 1;
		for (uint8_t child = 0; child < count; child++) {
			uint8_t val = 0;
			if (in.peekUInt8(val) == -1) {
				Log::error("Failed to peek at byte from input stream.");
				return;
			}
			if (val >> 7) {
				// Check if it's a leaf (both 2-byte and 8-byte start with 1)
				set(new Leaf(this, in));
			} else {
				set(new Branch(this, in));
			}
		}
		break;
	}
	case BRANCH_COLLAPSED: {
		uint8_t color = 0;
		if (in.readUInt8(color) == -1) {
			Log::error("Failed to read collapsed branch value from input stream.");
		}
		expandCollapsed(color);
		break;
	}
	default:
		Log::error("Invalid branch type in header");
		break;
	}
}

Branch::~Branch() {
	for (auto &child : _children) {
		if (child) {
			delete child;
		}
	}
	_children = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
}

void Branch::expandCollapsed(uint8_t color) {
	if (depth() == 15) {
		for (uint8_t i = 0; i < 8; i++) {
			set(new Leaf(this, i, color));
		}
	} else {
		for (uint8_t i = 0; i < 8; i++) {
			set(new Branch(this, i, color));
		}
	}
}

void Branch::write(io::WriteStream &out) const {
	if (!_parent && !first()) { // Empty model case
		// branch header
		for (int i = 0; i < 15; ++i) {
			out.writeUInt8(0);
		}
		out.writeUInt8(LEAF_2BYTE); // 2-byte payload leaf header
		out.writeUInt8(0);			// Both foreground and background zero
		out.writeUInt8(0);
		return;
	}
	uint8_t collapsedValue = tryCollapse();
	if (collapsedValue != 0) {
		out.writeUInt8(BRANCH_COLLAPSED | (_octant & 0b111)); // Header
		out.writeUInt8(collapsedValue);
		return;
	}
	out.writeUInt8(BRANCH_REGULAR | ((count() - 1) << 3) | (_octant & 0b111)); // Header
	for (uint8_t i = 0; i < 8; i++) {
		if (_children[i]) {
			_children[i]->write(out);
		}
	}
}

uint8_t Branch::count() const {
	uint8_t count = 0;
	for (uint8_t i = 0; i < 8; i++) {
		if (_children[i]) {
			count++;
		}
	}
	return count;
}

Node *Branch::first() const {
	for (uint8_t i = 0; i < 8; i++) {
		if (_children[i]) {
			return _children[i];
		}
	}
	return nullptr;
}

Node *Branch::nextValidChild(uint8_t previous) const {
	for (uint8_t i = previous + 1; i < 8; i++) {
		if (_children[i]) {
			return _children[i];
		}
	}
	return nullptr;
}

Node *Branch::operator[](uint8_t child) const {
	core_assert_msg(child < 8, "Child index out of bounds.");
	return _children[child];
}

Branch &Branch::operator=(Branch &&other) noexcept {
	if (this != &other) {
		_octant = other._octant;
		_parent = other._parent;
		for (uint8_t i = 0; i < 8; i++) {
			delete _children[i];
			_children[i] = other._children[i];
			other._children[i] = nullptr;
		}
	}
	return *this;
}

void Branch::set(Node *child) {
	core_assert_msg(child, "Child should not be nullptr.");
	uint8_t octant = child->getOctant();
	core_assert_msg(octant < 8, "Octant index out of bounds.");
	delete _children[octant];
	_children[octant] = child;
}

void Branch::remove(uint8_t child) {
	core_assert_msg(child < 8, "Child index out of bounds.");
	delete _children[child];
	_children[child] = nullptr;
	if (_parent && !first()) {
		_parent->remove(this->_octant);
	}
}

uint8_t Branch::tryCollapse() const {
	return tryCollapsing(tryCollapseGetColor());
}

uint8_t Branch::tryCollapsing(uint8_t color) const {
	if (color == 0) {
		return 0;
	}
	for (const auto &child : _children) {
		if (!child) {
			return 0;
		}
		if (isLeaf(child)) {
			Leaf *leaf = (Leaf *)child;
			for (uint8_t i = 0; i < 8; i++) {
				if ((*leaf)[i] != color) {
					return 0;
				}
			}
		} else if (isBranch(child)) {
			Branch *branch = (Branch *)child;
			if (branch->tryCollapsing(color) == 0) {
				return 0;
			}
		}
	}
	return color;
}

uint8_t Branch::tryCollapseGetColor() const {
	if (!_children[0]) {
		return 0;
	}
	if (isLeaf(_children[0])) {
		return (*(Leaf *)_children[0])[0];
	}
	if (isBranch(_children[0])) {
		return ((Branch *)_children[0])->tryCollapseGetColor();
	}
	return 0;
}

} // namespace BenVoxel
