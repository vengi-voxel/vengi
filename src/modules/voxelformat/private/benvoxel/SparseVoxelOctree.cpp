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
#include "SparseVoxelOctree.h"
namespace BenVoxel {
Voxel::Voxel(std::uint16_t x, std::uint16_t y, std::uint16_t z, std::uint8_t index) : Position(x, y, z), index(index) {
}
SparseVoxelOctree::SparseVoxelOctree() : root(), sizeX(UINT16_MAX), sizeY(UINT16_MAX), sizeZ(UINT16_MAX) {
}
SparseVoxelOctree::SparseVoxelOctree(std::uint16_t sizeX, std::uint16_t sizeY, std::uint16_t sizeZ)
	: root(), sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ) {
}
SparseVoxelOctree::SparseVoxelOctree(std::istream &in) : SparseVoxelOctree() {
	std::uint16_t dimensions[3] = {0, 0, 0};
	in.read(reinterpret_cast<char *>(dimensions), sizeof(dimensions));
	if (!in.good())
		throw std::runtime_error("Failed to read model dimensions.");
	sizeX = dimensions[0];
	sizeY = dimensions[1];
	sizeZ = dimensions[2];
	root = Branch(nullptr, in);
}
SparseVoxelOctree::SparseVoxelOctree(std::istream &in, std::uint16_t sizeX, std::uint16_t sizeY, std::uint16_t sizeZ)
	: SparseVoxelOctree(sizeX, sizeY, sizeZ) {
	root = Branch(nullptr, in);
}
SparseVoxelOctree::SparseVoxelOctree(std::list<Voxel> voxels, std::uint16_t sizeX, std::uint16_t sizeY,
									 std::uint16_t sizeZ)
	: SparseVoxelOctree(sizeX, sizeY, sizeZ) {
	for (const Voxel &voxel : voxels)
		set(voxel);
}
SparseVoxelOctree::SparseVoxelOctree(const SparseVoxelOctree &other)
	: SparseVoxelOctree(other.voxels(), other.getSizeX(), other.getSizeY(), other.getSizeZ()) {
}
void SparseVoxelOctree::write(std::ostream &out, bool includeSizes) const {
	if (includeSizes) {
		uint16_t dimensions[3] = {sizeX, sizeY, sizeZ};
		out.write(reinterpret_cast<const char *>(dimensions), sizeof(dimensions));
	}
	root.write(out);
}
std::uint8_t SparseVoxelOctree::operator[](Position &position) const {
	return get(position.x, position.y, position.z);
}
std::uint8_t SparseVoxelOctree::get(std::uint16_t x, std::uint16_t y, std::uint16_t z) const {
	Branch *branch = const_cast<Branch *>(&root);
	for (std::uint8_t level = 15; level > 1; level--) {
		Node *node = (*branch)[(z >> level & 1) << 2 | (y >> level & 1) << 1 | x >> level & 1];
		if (dynamic_cast<Branch *>(node))
			branch = dynamic_cast<Branch *>(node);
		else
			return 0;
	}
	Node *leaf = (*branch)[((z >> 1 & 1) << 2 | (y >> 1 & 1) << 1 | x >> 1 & 1)];
	if (dynamic_cast<Leaf *>(leaf))
		return (*dynamic_cast<Leaf *>(leaf))[(z & 1) << 2 | (y & 1) << 1 | x & 1];
	return 0;
}
std::list<Voxel> SparseVoxelOctree::voxels() const {
	std::list<Voxel> list = {};
	std::stack<Branch *> stack = {};
	fillStack(stack, const_cast<Branch *>(&root));
	while (!stack.empty()) {
		Branch *branch = stack.top();
		stack.pop();
		if (stack.size() == 14)
			for (uint8_t octant = 0; octant < 8; octant++) {
				Node *node = (*branch)[octant];
				if (dynamic_cast<Leaf *>(node)) {
					Leaf *leaf = dynamic_cast<Leaf *>(node);
					Position position = leaf->position();
					for (uint8_t octant = 0; octant < 8; octant++) {
						uint8_t index = (*leaf)[octant];
						if (index)
							list.push_back(Voxel(position.x + (octant & 1), position.y + ((octant >> 1) & 1),
												 position.z + ((octant >> 2) & 1), index));
					}
				}
			}
		Branch *parent = branch->getParent();
		if (parent) {
			Node *next = parent->nextValidChild(branch->getOctant());
			if (dynamic_cast<Branch *>(next))
				fillStack(stack, dynamic_cast<Branch *>(next));
		}
	}
	return list;
}
void SparseVoxelOctree::fillStack(std::stack<Branch *> &stack, Branch *branch) {
	while (branch) {
		stack.push(branch);
		branch = dynamic_cast<Branch *>(branch->first());
	}
}
void SparseVoxelOctree::set(Voxel voxel) {
	return set(voxel.x, voxel.y, voxel.z, voxel.index);
}
void SparseVoxelOctree::set(std::uint16_t x, std::uint16_t y, std::uint16_t z, std::uint8_t index) {
	Branch *branch = &root;
	std::uint8_t octant;
	for (std::uint8_t level = 15; level > 1; level--) {
		octant = (z >> level & 1) << 2 | (y >> level & 1) << 1 | x >> level & 1;
		Node *node = (*branch)[octant];
		if (dynamic_cast<Branch *>(node))
			branch = dynamic_cast<Branch *>(node);
		else {
			if (index == 0)
				return;
			branch->set(std::make_unique<Branch>(branch, octant));
			branch = dynamic_cast<Branch *>((*branch)[octant]);
		}
	}
	octant = (z >> 1 & 1) << 2 | (y >> 1 & 1) << 1 | x >> 1 & 1;
	Node *node = (*branch)[octant];
	Leaf *leaf = nullptr;
	if (!dynamic_cast<Leaf *>(node)) {
		if (index == 0)
			return;
		branch->set(std::make_unique<Leaf>(branch, octant));
		leaf = dynamic_cast<Leaf *>((*branch)[octant]);
	} else {
		leaf = dynamic_cast<Leaf *>(node);
	}
	leaf->set((z & 1) << 2 | (y & 1) << 1 | x & 1, index);
}
SparseVoxelOctree::~SparseVoxelOctree() {
	clear();
}
std::uint16_t SparseVoxelOctree::getSizeX() const {
	return sizeX;
}
std::uint16_t SparseVoxelOctree::getSizeY() const {
	return sizeY;
}
std::uint16_t SparseVoxelOctree::getSizeZ() const {
	return sizeZ;
}
void SparseVoxelOctree::clear() {
	root = Branch();
	sizeX = UINT16_MAX;
	sizeY = UINT16_MAX;
	sizeZ = UINT16_MAX;
}
} // namespace BenVoxel
