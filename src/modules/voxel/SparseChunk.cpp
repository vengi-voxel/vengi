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
			const int bit = ctz64(bits);
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
	_blocks.clear();
	_densePayloads.clear();
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
				const Voxel *payload = b == nullptr ? air : (b->dense() ? b->voxels : nullptr);
				const int first = voxelIndex(bx, by, bz);
				const int base = (bz - mins.z) * planeStride + (by - mins.y) * size.x + bx - mins.x;
				for (int z = 0; z < zEnd - bz; ++z) {
					for (int y = 0; y < yEnd - by; ++y) {
						Voxel *out = target + base + z * planeStride + y * size.x;
						const int idx = first + (z * BrickSide + y) * BrickSide;
						if (payload) {
							if (width == BrickSide) {
								memcpy((void *)out, payload + idx, BrickSide * sizeof(Voxel));
							} else {
								memcpy((void *)out, payload + idx, width * sizeof(Voxel));
							}
						} else {
							memcpy((void *)out, air, width * sizeof(Voxel));
							uint64_t bits = (b->mask[idx >> 6] >> (idx & 63)) & (((uint64_t)1 << width) - 1);
							int packed = packedIndex(b, idx);
							while (bits) {
								out[ctz64(bits)].overwrite(b->voxels[packed++]);
								bits &= bits - 1;
							}
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
		const int xmin = ctz64(rows);
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

} // namespace voxel
