/**
 * @file
 */

#include "SparseChunk.h"
#include <glm/common.hpp>

namespace voxel {

void SparseChunk::makeDense(Brick *b) {
	Voxel *dense = new Voxel[BrickVoxels];
	int packed = 0;
	for (int w = 0; w < BrickMaskWords; ++w) {
		uint64_t bits = b->mask[w];
		while (bits != 0u) {
			const int bit = core::countTrailingZeros(bits);
			bits &= bits - 1u;
			dense[(w << 6) | bit].overwrite(b->voxels[packed++]);
		}
	}
	if (b->voxels != b->inlineVoxels) {
		delete[] b->voxels;
	}
	b->voxels = dense;
	b->capacity = BrickVoxels;
	_densePayloads[_cells[b->cell] - 1] = dense;
}

bool SparseChunk::insertPacked(Brick *b, int idx, const Voxel &voxel) {
	if (b->count == b->capacity) {
		if (b->capacity >= PackedMaxVoxels) {
			makeDense(b);
			return setVoxel(b, idx, voxel);
		}
		const int newCapacity = b->capacity * 4 < PackedMaxVoxels ? b->capacity * 4 : PackedMaxVoxels;
		Voxel *grown = new Voxel[newCapacity];
		for (int i = 0; i < b->count; ++i) {
			grown[i].overwrite(b->voxels[i]);
		}
		if (b->voxels != b->inlineVoxels) {
			delete[] b->voxels;
		}
		b->voxels = grown;
		b->capacity = (uint16_t)newCapacity;
	}
	const int pos = packedIndex(b, idx);
	for (int i = b->count; i > pos; --i) {
		b->voxels[i].overwrite(b->voxels[i - 1]);
	}
	b->voxels[pos].overwrite(voxel);
	const int w = idx >> 6;
	b->mask[w] |= (uint64_t)1u << (idx & 63);
	for (int i = w + 1; i < BrickMaskWords; ++i) {
		++b->prefix[i];
	}
	++b->count;
	++_voxelCount;
	return true;
}

SparseChunk::~SparseChunk() {
	clear();
}

void SparseChunk::clear() {
	for (int i = 0; i < _poolUsed; ++i) {
		Brick *b = poolBrick(i);
		if (b->voxels != nullptr && b->voxels != b->inlineVoxels) {
			delete[] b->voxels;
		}
	}
	for (Brick *block : _blocks) {
		delete[] block;
	}
	_blocks.release();
	_densePayloads.release();
	_poolUsed = 0;
	_freeBrick = -1;
	delete[] _cells;
	_cells = nullptr;
	_voxelCount = 0;
}

SparseChunk::Brick *SparseChunk::createBrick(int x, int y, int z) {
	if (_cells == nullptr) {
		_cells = new uint16_t[Bricks]();
	}
	int i;
	if (_freeBrick >= 0) {
		i = _freeBrick;
		_freeBrick = poolBrick(i)->count;
	} else {
		if (_poolUsed == (int)_blocks.size() * PoolBlockBricks) {
			_blocks.push_back(new Brick[PoolBlockBricks]);
		}
		i = _poolUsed++;
		_densePayloads.push_back(nullptr);
	}
	Brick *b = poolBrick(i);
	for (int w = 0; w < BrickMaskWords; ++w) {
		b->mask[w] = 0u;
		b->prefix[w] = 0u;
	}
	b->count = 0;
	b->cell = (uint16_t)brickCellIndex(x, y, z);
	if (_denseBricks) {
		b->voxels = new Voxel[BrickVoxels];
		b->capacity = BrickVoxels;
	} else {
		b->voxels = b->inlineVoxels;
		b->capacity = InlineVoxels;
	}
	_cells[b->cell] = (uint16_t)(i + 1);
	_densePayloads[i] = _denseBricks ? b->voxels : nullptr;
	return b;
}

void SparseChunk::releaseBrick(int x, int y, int z) {
	uint16_t &cell = _cells[brickCellIndex(x, y, z)];
	const int i = (int)cell - 1;
	Brick *b = poolBrick(i);
	if (b->voxels != b->inlineVoxels) {
		delete[] b->voxels;
	}
	b->voxels = nullptr;
	b->count = _freeBrick;
	_freeBrick = i;
	_densePayloads[i] = nullptr;
	cell = 0u;
}

bool SparseChunk::removeVoxel(int x, int y, int z, bool releaseMemory) {
	Brick *brick = this->brick(x, y, z);
	if (brick == nullptr) {
		return false;
	}
	const int idx = voxelIndex(x, y, z);
	const uint64_t bit = (uint64_t)1u << (idx & 63);
	uint64_t &word = brick->mask[idx >> 6];
	if ((word & bit) == 0u) {
		return false;
	}
	if (brick->dense()) {
		brick->voxels[idx].overwrite(Voxel());
	} else {
		const int pos = packedIndex(brick, idx);
		for (int i = pos + 1; i < brick->count; ++i) {
			brick->voxels[i - 1].overwrite(brick->voxels[i]);
		}
		for (int i = (idx >> 6) + 1; i < BrickMaskWords; ++i) {
			--brick->prefix[i];
		}
	}
	word &= ~bit;
	--_voxelCount;
	if (--brick->count == 0 && releaseMemory) {
		releaseBrick(x, y, z);
	}
	return true;
}

int SparseChunk::setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel) {
	int added = 0;
	// setVoxel() already accounts these in _voxelCount
	int addedPacked = 0;
	const int endX = x + count;
	while (x < endX) {
		Brick *brick = findOrCreateBrick(x, y, z);
		const int brickEndX = (x & ~(BrickSide - 1)) + BrickSide;
		const int segmentEndX = endX < brickEndX ? endX : brickEndX;
		const int rowIdx = voxelIndex(x, y, z);
		if (!brick->dense()) {
			for (int i = 0, n = segmentEndX - x; i < n; ++i) {
				if (setVoxel(brick, rowIdx + i, voxel)) {
					++addedPacked;
				}
			}
			x = segmentEndX;
			continue;
		}
		uint64_t &word = brick->mask[rowIdx >> 6];
		int brickAdded = 0;
		for (int i = 0, n = segmentEndX - x; i < n; ++i) {
			const int idx = rowIdx + i;
			const uint64_t bit = (uint64_t)1u << (idx & 63);
			if (word & bit) {
				brick->voxels[idx] = voxel;
			} else {
				brick->voxels[idx].overwrite(voxel);
				word |= bit;
				++brickAdded;
			}
		}
		brick->count += brickAdded;
		added += brickAdded;
		x = segmentEndX;
	}
	_voxelCount += added;
	return added + addedPacked;
}

const Voxel *SparseChunk::unpackRow(uint64_t bits, const Voxel *v, int width, Voxel *scratch) {
	memcpy((void *)scratch, EmptyRow, width * sizeof(Voxel));
	while (bits != 0) {
		scratch[core::countTrailingZeros(bits)].overwrite(*v++);
		bits &= bits - 1;
	}
	return scratch;
}

void SparseChunk::readRegion(const glm::ivec3 &mins, const glm::ivec3 &size, Voxel *target) const {
	const glm::ivec3 end = mins + size;
	core_assert(glm::all(glm::greaterThanEqual(mins, glm::ivec3(0))));
	core_assert(glm::all(glm::greaterThan(size, glm::ivec3(0))));
	core_assert(glm::all(glm::lessThanEqual(end, glm::ivec3(Side))));
	const int planeStride = size.x * size.y;
	for (int bz = mins.z; bz < end.z;) {
		const int zEnd = glm::min(end.z, (bz | (BrickSide - 1)) + 1);
		for (int by = mins.y; by < end.y;) {
			const int yEnd = glm::min(end.y, (by | (BrickSide - 1)) + 1);
			for (int bx = mins.x; bx < end.x;) {
				const int xEnd = glm::min(end.x, (bx | (BrickSide - 1)) + 1);
				const int width = xEnd - bx;
				const Brick *b = brick(bx, by, bz);
				static constexpr Voxel air[BrickVoxels]{};
				const Voxel *payload = b ? (b->dense() ? b->voxels : nullptr) : air;
				const int first = voxelIndex(bx, by, bz);
				const int base = (bz - mins.z) * planeStride + (by - mins.y) * size.x + bx - mins.x;
				for (int z = 0; z < zEnd - bz; ++z) {
					for (int y = 0; y < yEnd - by; ++y) {
						Voxel *out = target + base + z * planeStride + y * size.x;
						const int idx = first + (z * BrickSide + y) * BrickSide;
						Voxel scratch[BrickSide];
						const Voxel *row = payload ? payload + idx : packedRow(b, idx, width, scratch);
						if (width == BrickSide) {
							memcpy((void *)out, row, BrickSide * sizeof(Voxel));
						} else {
							memcpy((void *)out, row, width * sizeof(Voxel));
						}
					}
				}
				bx = xEnd;
			}
			by = yEnd;
		}
		bz = zEnd;
	}
}

bool SparseChunk::calculateBounds(glm::ivec3 &mins, glm::ivec3 &maxs) const {
	bool found = false;
	for (int i = 0; i < _poolUsed; ++i) {
		const Brick *brick = poolBrick(i);
		if (brick->voxels == nullptr || brick->count == 0) {
			continue;
		}
		uint64_t slices = 0u;
		int zmin = BrickSide;
		int zmax = -1;
		for (int w = 0; w < BrickMaskWords; ++w) {
			if (brick->mask[w] == 0u) {
				continue;
			}
			if (zmin == BrickSide) {
				zmin = w;
			}
			zmax = w;
			slices |= brick->mask[w];
		}
		uint32_t rows = 0u;
		int ymin = BrickSide;
		int ymax = -1;
		for (int row = 0; row < BrickSide; ++row) {
			const uint32_t xbits = (uint32_t)((slices >> (row * BrickSide)) & 0xFFu);
			if (xbits == 0u) {
				continue;
			}
			if (ymin == BrickSide) {
				ymin = row;
			}
			ymax = row;
			rows |= xbits;
		}
		const int xmin = core::countTrailingZeros(rows);
		int xmax = BrickSide - 1;
		while ((rows & (1u << xmax)) == 0u) {
			--xmax;
		}
		const int bx = (brick->cell & (BricksPerAxis - 1)) << BrickShift;
		const int by = ((brick->cell >> BricksPerAxisShift) & (BricksPerAxis - 1)) << BrickShift;
		const int bz = (brick->cell >> (2 * BricksPerAxisShift)) << BrickShift;
		const glm::ivec3 bmins(bx + xmin, by + ymin, bz + zmin);
		const glm::ivec3 bmaxs(bx + xmax, by + ymax, bz + zmax);
		if (!found) {
			mins = bmins;
			maxs = bmaxs;
			found = true;
		} else {
			mins = (glm::min)(mins, bmins);
			maxs = (glm::max)(maxs, bmaxs);
		}
	}
	return found;
}

SparseChunk::PreparedBrick SparseChunk::prepareBrick(int bx, int by, int bz, const Voxel (&tile)[BrickVoxels],
													 const uint64_t (&mask)[BrickMaskWords], int count) {
	PreparedBrick pb;
	pb.cell = (uint16_t)brickCellIndex(bx, by, bz);
	pb.count = count;
	if (count <= 0) {
		return pb;
	}
	for (int w = 0; w < BrickMaskWords; ++w) {
		pb.mask[w] = mask[w];
	}
	if (count > PackedMaxVoxels) {
		pb.capacity = BrickVoxels;
		pb.heapVoxels = new Voxel[BrickVoxels];
		memcpy((void *)pb.heapVoxels, (const void *)tile, (size_t)BrickVoxels * sizeof(Voxel));
		return pb;
	}
	uint16_t cap = InlineVoxels;
	while ((int)cap < count && cap < PackedMaxVoxels) {
		cap = (uint16_t)(cap * 4u < (uint16_t)PackedMaxVoxels ? cap * 4u : (uint16_t)PackedMaxVoxels);
	}
	pb.capacity = cap;
	Voxel *dst = pb.inlineVoxels;
	if (cap > InlineVoxels) {
		pb.heapVoxels = new Voxel[cap];
		dst = pb.heapVoxels;
	}
	int packed = 0;
	for (int w = 0; w < BrickMaskWords; ++w) {
		pb.prefix[w] = (uint8_t)packed;
		uint64_t bits = mask[w];
		while (bits != 0u) {
			const int bit = core::countTrailingZeros(bits);
			bits &= bits - 1u;
			dst[packed++].overwrite(tile[(w << 6) | bit]);
		}
	}
	return pb;
}

void SparseChunk::adoptPreparedBrick(PreparedBrick &prepared) {
	if (prepared.count <= 0) {
		if (_cells != nullptr && _cells[prepared.cell] != 0u) {
			const int i = (int)_cells[prepared.cell] - 1;
			Brick *b = poolBrick(i);
			_voxelCount -= b->count;
			if (b->voxels != nullptr && b->voxels != b->inlineVoxels) {
				delete[] b->voxels;
			}
			b->voxels = nullptr;
			b->count = _freeBrick;
			_freeBrick = i;
			_densePayloads[i] = nullptr;
			_cells[prepared.cell] = 0u;
		}
		prepared.destroy();
		return;
	}
	if (_cells == nullptr) {
		_cells = new uint16_t[Bricks]();
	}
	int i = (int)_cells[prepared.cell] - 1;
	Brick *b = nullptr;
	if (i >= 0) {
		b = poolBrick(i);
		_voxelCount -= b->count;
		if (b->voxels != nullptr && b->voxels != b->inlineVoxels) {
			delete[] b->voxels;
		}
	} else {
		if (_freeBrick >= 0) {
			i = _freeBrick;
			_freeBrick = poolBrick(i)->count;
		} else {
			if (_poolUsed == (int)_blocks.size() * PoolBlockBricks) {
				_blocks.push_back(new Brick[PoolBlockBricks]);
			}
			i = _poolUsed++;
			_densePayloads.push_back(nullptr);
		}
		b = poolBrick(i);
		_cells[prepared.cell] = (uint16_t)(i + 1);
	}
	for (int w = 0; w < BrickMaskWords; ++w) {
		b->mask[w] = prepared.mask[w];
		b->prefix[w] = prepared.prefix[w];
	}
	b->count = prepared.count;
	b->capacity = prepared.capacity;
	b->cell = prepared.cell;
	if (prepared.heapVoxels != nullptr) {
		b->voxels = prepared.heapVoxels;
		prepared.heapVoxels = nullptr;
	} else {
		b->voxels = b->inlineVoxels;
		memcpy((void *)b->inlineVoxels, (const void *)prepared.inlineVoxels, sizeof(b->inlineVoxels));
	}
	_densePayloads[i] = b->dense() ? b->voxels : nullptr;
	_voxelCount += b->count;
	prepared.count = 0;
}

SparseChunk *SparseChunk::clone() const {
	SparseChunk *copy = new SparseChunk(_denseBricks);
	if (_cells == nullptr || _voxelCount == 0) {
		return copy;
	}
	copy->_cells = new uint16_t[Bricks]();
	copy->_voxelCount = _voxelCount;
	for (int i = 0; i < _poolUsed; ++i) {
		const Brick *src = poolBrick(i);
		if (src->voxels == nullptr || src->count == 0) {
			continue;
		}
		if (copy->_poolUsed == (int)copy->_blocks.size() * PoolBlockBricks) {
			copy->_blocks.push_back(new Brick[PoolBlockBricks]);
		}
		const int dstIdx = copy->_poolUsed++;
		Brick *dst = copy->poolBrick(dstIdx);
		for (int w = 0; w < BrickMaskWords; ++w) {
			dst->mask[w] = src->mask[w];
			dst->prefix[w] = src->prefix[w];
		}
		dst->count = src->count;
		dst->capacity = src->capacity;
		dst->cell = src->cell;
		if (src->voxels == src->inlineVoxels) {
			dst->voxels = dst->inlineVoxels;
			memcpy((void *)dst->inlineVoxels, (const void *)src->inlineVoxels, sizeof(src->inlineVoxels));
			copy->_densePayloads.push_back(nullptr);
		} else {
			dst->voxels = new Voxel[src->capacity];
			memcpy((void *)dst->voxels, (const void *)src->voxels, (size_t)src->capacity * sizeof(Voxel));
			copy->_densePayloads.push_back(src->dense() ? dst->voxels : nullptr);
		}
		copy->_cells[dst->cell] = (uint16_t)(dstIdx + 1);
	}
	return copy;
}

size_t SparseChunk::allocatedBytes() const {
	size_t total = 0u;
	if (_cells != nullptr) {
		total += (size_t)Bricks * sizeof(uint16_t);
	}
	total += _blocks.size() * (size_t)PoolBlockBricks * sizeof(Brick);
	total += _densePayloads.capacity() * sizeof(const Voxel *);
	for (int i = 0; i < _poolUsed; ++i) {
		const Brick *b = poolBrick(i);
		if (b->voxels != nullptr && b->voxels != b->inlineVoxels) {
			total += (size_t)b->capacity * sizeof(Voxel);
		}
	}
	return total;
}

} // namespace voxel
