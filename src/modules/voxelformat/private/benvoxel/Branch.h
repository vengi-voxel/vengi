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

#include "Node.h"
#include "core/collection/Array.h"
#include "io/Stream.h"

namespace BenVoxel {

class Branch : public Node {
protected:
	core::Array<Node *, 8> _children{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

public:
	Branch();
	Branch(Branch *parent, uint8_t octant);
	Branch(Branch *parent, uint8_t octant, uint8_t color);
	Branch(Branch *parent, io::SeekableReadStream &in);
	virtual ~Branch();
	void write(io::SeekableWriteStream &out) const override;
	uint8_t count() const;
	Node *first() const;
	Node *nextValidChild(uint8_t previous) const;
	Node *operator[](uint8_t child) const;
	Branch &operator=(Branch &&other) noexcept;
	void set(Node *child);
	void remove(uint8_t child);
	void expandCollapsed(uint8_t color);
	uint8_t tryCollapse() const;
	uint8_t tryCollapsing(uint8_t color) const;
	uint8_t tryCollapseGetColor() const;
};

} // namespace BenVoxel
