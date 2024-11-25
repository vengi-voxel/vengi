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

#include "Branch.h"
#include "core/collection/DynamicStack.h"
#include "io/Stream.h"

namespace BenVoxel {

struct SVOVoxel {
	const Position position;
	const uint8_t index;
	SVOVoxel(uint16_t x, uint16_t y, uint16_t z, uint8_t index);
};

class SparseVoxelOctree {
protected:
	Branch _root;
	uint16_t _sizeX;
	uint16_t _sizeY;
	uint16_t _sizeZ;
	static void fillStack(core::DynamicStack<const Branch *> &stack, const Branch *branch);

public:
	SparseVoxelOctree(uint16_t sizeX, uint16_t sizeY, uint16_t sizeZ);
	SparseVoxelOctree(io::SeekableReadStream &in, uint16_t sizeX, uint16_t sizeY, uint16_t sizeZ);
	~SparseVoxelOctree();
	void write(io::WriteStream &out, bool includeSizes = true) const;
	uint8_t operator[](Position &position) const;
	uint8_t get(uint16_t x, uint16_t y, uint16_t z) const;
	void set(SVOVoxel voxel);
	void set(uint16_t x, uint16_t y, uint16_t z, uint8_t index);
	core::DynamicArray<SVOVoxel> voxels() const;
	void clear();
};

} // namespace BenVoxel
