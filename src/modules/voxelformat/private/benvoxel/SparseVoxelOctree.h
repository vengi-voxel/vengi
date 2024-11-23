#pragma once
#include "Branch.h"
#include <iostream>
#include <list>
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
struct Voxel : Position {
	const std::uint8_t index;
	Voxel(std::uint16_t x, std::uint16_t y, std::uint16_t z, std::uint8_t index);
};
class SparseVoxelOctree {
protected:
	Branch root;
	std::uint16_t sizeX;
	std::uint16_t sizeY;
	std::uint16_t sizeZ;
	static void fillStack(std::stack<Branch *> &stack, Branch *branch);

public:
	SparseVoxelOctree();
	SparseVoxelOctree(std::uint16_t sizeX, std::uint16_t sizeY, std::uint16_t sizeZ);
	SparseVoxelOctree(std::istream &in);
	SparseVoxelOctree(std::istream &in, std::uint16_t sizeX, std::uint16_t sizeY, std::uint16_t sizeZ);
	SparseVoxelOctree(std::list<Voxel> voxels, std::uint16_t sizeX = UINT16_MAX, std::uint16_t sizeY = UINT16_MAX,
					  std::uint16_t sizeZ = UINT16_MAX);
	SparseVoxelOctree(const SparseVoxelOctree &other);
	virtual ~SparseVoxelOctree();
	void write(std::ostream &out, bool includeSizes = true) const;
	std::uint8_t operator[](Position &position) const;
	std::uint16_t getSizeX() const;
	std::uint16_t getSizeY() const;
	std::uint16_t getSizeZ() const;
	std::uint8_t get(std::uint16_t x, std::uint16_t y, std::uint16_t z) const;
	void set(Voxel voxel);
	void set(std::uint16_t x, std::uint16_t y, std::uint16_t z, std::uint8_t index);
	std::list<Voxel> voxels() const;
	void clear();
};
} // namespace BenVoxel
