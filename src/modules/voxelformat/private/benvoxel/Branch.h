#pragma once
#include "Leaf.h"
#include "Node.h"
#include <array>
#include <iostream>
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
class Branch : public Node {
protected:
	std::array<std::unique_ptr<Node>, 8> children;

public:
	Branch();
	Branch(Branch *parent, std::uint8_t octant);
	Branch(Branch *parent, std::uint8_t octant, std::uint8_t color);
	Branch(Branch *parent, std::istream &in);
	virtual ~Branch() override = default;
	void write(std::ostream &out) const override;
	std::uint8_t count() const;
	Node *first() const;
	Node *nextValidChild(std::uint8_t previous) const;
	Node *operator[](std::uint8_t child) const;
	Branch &operator=(Branch &&other) noexcept;
	void set(std::unique_ptr<Node> child);
	void remove(std::uint8_t child);
	void expandCollapsed(std::uint8_t color);
	std::uint8_t tryCollapse() const;
	std::uint8_t tryCollapsing(std::uint8_t color) const;
	std::uint8_t tryCollapseGetColor() const;
};
} // namespace BenVoxel
