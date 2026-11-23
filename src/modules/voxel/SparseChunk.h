/**
 * @file
 */

#pragma once

#include "core/Bits.h"
#include "core/NonCopyable.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>
#include <stdint.h>

namespace voxel {

/**
 * @brief Voxel storage for one 256^3 chunk of a sparse volume.
 *
 * The chunk is split into 32^3 bricks of 8^3 voxels. A flat table maps every brick cell to a brick of the chunk's
 * brick pool - bricks are only allocated if they contain voxels. A brick has an occupancy bitmask and stores its
 * voxels either packed or dense:
 * - packed: only the occupied voxels in bit order - a few of them inline in the brick, more in a small heap array.
 *   This keeps scattered voxels cheap.
 * - dense: all 512 slots - slots without the occupancy bit set hold the default (air) voxel. A packed brick becomes
 *   dense once it would exceed @c PackedMaxVoxels and stays dense until it is released.
 *
 * Bricks never move in memory until they are released.
 *
 * Coordinates are local to the chunk and must be in the range [0, Side).
 *
 * This is not thread safe.
 */
class SparseChunk : public core::NonCopyable {
public:
	static constexpr int SideShift = 8;
	static constexpr int Side = 1 << SideShift;
	static constexpr int BrickShift = 3;
	static constexpr int BrickSide = 1 << BrickShift;
	static constexpr int BrickVoxels = BrickSide * BrickSide * BrickSide;
	static constexpr int BrickMaskWords = BrickVoxels / 64;
	static constexpr int BricksPerAxisShift = SideShift - BrickShift;
	static constexpr int BricksPerAxis = 1 << BricksPerAxisShift;
	static constexpr int Bricks = BricksPerAxis * BricksPerAxis * BricksPerAxis;
	static constexpr int InlineVoxels = 4;
	static constexpr int PackedMaxVoxels = 64;
	static constexpr int PoolBlockShift = 6;
	static constexpr int PoolBlockBricks = 1 << PoolBlockShift;

	static_assert(BrickSide == 8, "The occupancy mask layout expects 8^3 bricks");
	static_assert(Bricks < 65536, "brick cells and pool indices are stored in 16 bits");
	static_assert(PackedMaxVoxels < 256, "prefix counts are stored in 8 bits");

	struct Brick {
		/** one word per z slice - bit index is y * 8 + x */
		uint64_t mask[BrickMaskWords];
		/** points to @c inlineVoxels, to a packed heap array or to the dense heap array - @c nullptr if released */
		Voxel *voxels;
		/** number of voxels - the next free pool slot for released bricks */
		int count;
		/** @c BrickVoxels for dense bricks */
		uint16_t capacity;
		/** the brick cell as returned by @c brickCellIndex() */
		uint16_t cell;
		/** packed bricks only: number of occupied voxels in the mask words before the given word */
		uint8_t prefix[BrickMaskWords];
		Voxel inlineVoxels[InlineVoxels];

		inline bool dense() const {
			return capacity == BrickVoxels;
		}
	};

	/**
	 * @brief Lock-free pre-packed or dense brick ready for O(1) pointer adoption into a SparseChunk.
	 */
	struct PreparedBrick {
		uint64_t mask[BrickMaskWords]{};
		Voxel *heapVoxels = nullptr;
		int count = 0;
		uint16_t capacity = 0u;
		uint16_t cell = 0u;
		uint8_t prefix[BrickMaskWords]{};
		Voxel inlineVoxels[InlineVoxels]{};

		inline void destroy() {
			delete[] heapVoxels;
			heapVoxels = nullptr;
			count = 0;
			capacity = 0u;
		}
	};

	// the build doesn't enable the popcnt instruction - the builtin would be a library call
	static inline int popcount64(uint64_t v) {
		v = v - ((v >> 1) & 0x5555555555555555ull);
		v = (v & 0x3333333333333333ull) + ((v >> 2) & 0x3333333333333333ull);
		return (int)((((v + (v >> 4)) & 0x0F0F0F0F0F0F0F0Full) * 0x0101010101010101ull) >> 56);
	}

	/** returned for positions in packed bricks that don't have a voxel */
	static constexpr Voxel EmptyVoxel{};

private:
	static constexpr Voxel EmptyRow[BrickSide]{};

	/** per brick cell: pool index + 1 or 0 if there is no brick - allocated with the first brick */
	uint16_t *_cells = nullptr;
	core::DynamicArray<Brick *> _blocks;
	/** Dense payloads by pool index, so lookups can bypass the brick metadata. */
	core::DynamicArray<const Voxel *> _densePayloads;
	int _poolUsed = 0;
	int _freeBrick = -1;
	int _voxelCount = 0;
	const bool _denseBricks;

	/** number of occupied voxels before @c idx in a packed brick */
	static inline int packedIndex(const Brick *b, int idx) {
		const int w = idx >> 6;
		return b->prefix[w] + popcount64(b->mask[w] & ((((uint64_t)1u) << (idx & 63)) - 1u));
	}

	static inline const Voxel &packedVoxel(const Brick *b, int idx) {
		const uint64_t word = b->mask[idx >> 6];
		const uint64_t bit = (uint64_t)1 << (idx & 63);
		if ((word & bit) == 0) {
			return EmptyVoxel;
		}
		return b->voxels[b->prefix[idx >> 6] + popcount64(word & (bit - 1))];
	}

	static const Voxel *unpackRow(uint64_t bits, const Voxel *v, int width, Voxel *scratch);

	CORE_FORCE_INLINE static const Voxel *packedRow(const Brick *b, int idx, int width, Voxel *scratch) {
		const uint64_t rowMask = ((uint64_t)1 << width) - 1;
		const uint64_t word = b->mask[idx >> 6];
		const int offset = idx & 63;
		const uint64_t bits = (word >> offset) & rowMask;
		if (bits == 0) {
			return EmptyRow;
		}
		if (bits != rowMask && scratch == nullptr) {
			return nullptr;
		}
		const Voxel *v = b->voxels + b->prefix[idx >> 6] + popcount64(word & (((uint64_t)1 << offset) - 1));
		return bits == rowMask ? v : unpackRow(bits, v, width, scratch);
	}

	inline Brick *poolBrick(int i) const {
		return _blocks[i >> PoolBlockShift] + (i & (PoolBlockBricks - 1));
	}

	Brick *createBrick(int x, int y, int z);
	void releaseBrick(int x, int y, int z);
	bool insertPacked(Brick *b, int idx, const Voxel &voxel);
	void makeDense(Brick *b);

public:
	/**
	 * @param denseBricks Always use dense bricks. Slots of dense bricks never move - references to them stay valid as
	 * long as the brick isn't released.
	 */
	explicit SparseChunk(bool denseBricks = false) : _denseBricks(denseBricks) {
	}
	~SparseChunk();

	static inline int brickCellIndex(int x, int y, int z) {
		return (((z >> BrickShift) << BricksPerAxisShift) + (y >> BrickShift)) * BricksPerAxis + (x >> BrickShift);
	}

	static inline int voxelIndex(int x, int y, int z) {
		constexpr int mask = BrickSide - 1;
		return ((z & mask) * BrickSide + (y & mask)) * BrickSide + (x & mask);
	}

	inline Brick *brick(int x, int y, int z) const {
		if (_cells == nullptr) {
			return nullptr;
		}
		const int i = _cells[brickCellIndex(x, y, z)];
		if (i == 0) {
			return nullptr;
		}
		return poolBrick(i - 1);
	}

	inline Brick *findOrCreateBrick(int x, int y, int z) {
		Brick *b = brick(x, y, z);
		if (b != nullptr) {
			return b;
		}
		return createBrick(x, y, z);
	}

	/**
	 * @brief Store a voxel in a brick of this chunk
	 * @param idx The index as returned by @c voxelIndex()
	 * @note Uses the assignment semantics of @c Voxel for already stored voxels
	 * @return @c true if no voxel was stored at this position before
	 */
	inline bool setVoxel(Brick *b, int idx, const Voxel &voxel) {
		const uint64_t bit = (uint64_t)1u << (idx & 63);
		uint64_t &word = b->mask[idx >> 6];
		if (word & bit) {
			b->voxels[b->dense() ? idx : packedIndex(b, idx)] = voxel;
			return false;
		}
		if (!b->dense()) {
			return insertPacked(b, idx, voxel);
		}
		// free slots hold the default voxel - the normal merging of the assignment operator is a no-op here
		b->voxels[idx].overwrite(voxel);
		word |= bit;
		++b->count;
		++_voxelCount;
		return true;
	}

	/**
	 * @brief Store a voxel using full field replacement (overwrite) instead of Voxel::operator=
	 * @return @c true if no voxel was stored at this position before
	 */
	inline bool setVoxelOverwrite(Brick *b, int idx, const Voxel &voxel) {
		const uint64_t bit = (uint64_t)1u << (idx & 63);
		uint64_t &word = b->mask[idx >> 6];
		if (word & bit) {
			b->voxels[b->dense() ? idx : packedIndex(b, idx)].overwrite(voxel);
			return false;
		}
		if (!b->dense()) {
			return insertPacked(b, idx, voxel);
		}
		b->voxels[idx].overwrite(voxel);
		word |= bit;
		++b->count;
		++_voxelCount;
		return true;
	}

	/**
	 * @return Pointer to the stored voxel if present (occupancy bit set), or @c nullptr otherwise.
	 */
	CORE_FORCE_INLINE const Voxel *findVoxel(int x, int y, int z) const {
		if (_cells == nullptr) {
			return nullptr;
		}
		const int i = _cells[brickCellIndex(x, y, z)] - 1;
		if (i < 0) {
			return nullptr;
		}
		const Brick *b = poolBrick(i);
		const int idx = voxelIndex(x, y, z);
		const uint64_t word = b->mask[idx >> 6];
		const uint64_t bit = (uint64_t)1u << (idx & 63);
		if ((word & bit) == 0u) {
			return nullptr;
		}
		if (b->dense()) {
			return b->voxels + idx;
		}
		return b->voxels + b->prefix[idx >> 6] + popcount64(word & (bit - 1u));
	}

	/**
	 * @return The stored voxel or @c EmptyVoxel
	 * @note References into packed bricks are invalidated by modifications of the brick
	 */
	static inline const Voxel &voxel(const Brick *b, int idx) {
		return b->dense() ? b->voxels[idx] : packedVoxel(b, idx);
	}

	/**
	 * @return The voxel or @c nullptr if no brick is allocated for the given position. Positions without a voxel return
	 * the default voxel.
	 * @note Pointers into packed bricks are invalidated by modifications of the brick
	 */
	CORE_FORCE_INLINE const Voxel *voxel(int x, int y, int z) const {
		if (_cells == nullptr) {
			return nullptr;
		}
		const int i = _cells[brickCellIndex(x, y, z)] - 1;
		if (i < 0) {
			return nullptr;
		}
		const int idx = voxelIndex(x, y, z);
		const Voxel *dense = _densePayloads.data()[i];
		return dense ? dense + idx : &packedVoxel(poolBrick(i), idx);
	}

	/**
	 * @brief Read the BrickSide consecutive voxels starting at a brick-aligned x.
	 * The returned row can refer to the chunk, shared air, or scratch. Use it before modifying the chunk
	 * or reusing scratch.
	 */
	CORE_FORCE_INLINE const Voxel *readBrickRow(int x, int y, int z, Voxel (&scratch)[BrickSide]) const {
		core_assert((x & (BrickSide - 1)) == 0);
		const int cell = _cells ? _cells[brickCellIndex(x, y, z)] : 0;
		if (cell == 0) {
			return EmptyRow;
		}
		const int idx = voxelIndex(0, y, z);
		const Voxel *dense = _densePayloads.data()[cell - 1];
		return dense ? dense + idx : packedRow(poolBrick(cell - 1), idx, BrickSide, scratch);
	}

	/** Borrow a contiguous row or return nullptr if it has to be unpacked. */
	static CORE_FORCE_INLINE const Voxel *borrowBrickRow(const Brick *brick, int idx) {
		const int row = idx & ~(BrickSide - 1);
		if (brick == nullptr) {
			return EmptyRow;
		}
		return brick->dense() ? brick->voxels + row : packedRow(brick, row, BrickSide, nullptr);
	}

	inline bool hasVoxel(int x, int y, int z) const {
		const Brick *b = brick(x, y, z);
		if (b == nullptr) {
			return false;
		}
		const int idx = voxelIndex(x, y, z);
		return (b->mask[idx >> 6] >> (idx & 63)) & 1u;
	}

	/**
	 * @note Uses the assignment semantics of @c Voxel for already stored voxels
	 * @return @c true if no voxel was stored at this position before
	 */
	inline bool setVoxel(int x, int y, int z, const Voxel &voxel) {
		return setVoxel(findOrCreateBrick(x, y, z), voxelIndex(x, y, z), voxel);
	}

	/**
	 * @param releaseMemory release bricks that became empty. If this is @c false and the chunk uses dense bricks,
	 * references to voxel slots stay valid until the chunk is cleared or destroyed.
	 * @return @c true if a voxel was stored at this position
	 */
	bool removeVoxel(int x, int y, int z, bool releaseMemory = true);

	/**
	 * @brief Store @c count voxels along the x axis - @c x + @c count must not exceed @c Side
	 * @return The amount of voxels that were not stored before
	 */
	int setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel);

	/**
	 * @brief Read a box wholly within the chunk into a tightly packed, X-major buffer.
	 * Unstored cells are copied as default air and stored voxels retain all their fields.
	 */
	void readRegion(const glm::ivec3 &mins, const glm::ivec3 &size, Voxel *target) const;

	void clear();

	/**
	 * @brief Lock-free builder that packs or copies an 8^3 staging tile into a PreparedBrick.
	 */
	static PreparedBrick prepareBrick(int bx, int by, int bz, const Voxel (&tile)[BrickVoxels],
									  const uint64_t (&mask)[BrickMaskWords], int count);

	/**
	 * @brief Adopt a PreparedBrick, transferring heap payloads and copying inline payloads.
	 */
	void adoptPreparedBrick(PreparedBrick &prepared);

	/**
	 * @brief Deep-copy this chunk including its brick pool and cell directory.
	 */
	SparseChunk *clone() const;

	/**
	 * @brief Allocated bytes for cell table, brick pool blocks, dense pointer table and brick voxel arrays.
	 */
	size_t allocatedBytes() const;

	inline int size() const {
		return _voxelCount;
	}

	inline int brickPoolUsed() const {
		return _poolUsed;
	}

	inline bool empty() const {
		return _voxelCount == 0;
	}

	/**
	 * @brief Calculate the local bounds of all stored voxels
	 * @return @c false if the chunk doesn't contain any voxel
	 */
	bool calculateBounds(glm::ivec3 &mins, glm::ivec3 &maxs) const;

	/**
	 * @brief Visit every live brick in the chunk with its local origin (bx, by, bz) and brick reference.
	 */
	template<class FUNC>
	void visitBricks(FUNC &&func) const {
		for (int i = 0; i < _poolUsed; ++i) {
			const Brick *b = poolBrick(i);
			if (b->voxels == nullptr || b->count == 0) {
				continue;
			}
			const int bx = (b->cell & (BricksPerAxis - 1)) << BrickShift;
			const int by = ((b->cell >> BricksPerAxisShift) & (BricksPerAxis - 1)) << BrickShift;
			const int bz = (b->cell >> (2 * BricksPerAxisShift)) << BrickShift;
			func(bx, by, bz, *b);
		}
	}

	template<class FUNC>
	void visitBricksMutable(FUNC &&func) {
		for (int i = 0; i < _poolUsed; ++i) {
			Brick *b = poolBrick(i);
			if (b->voxels == nullptr || b->count == 0) {
				continue;
			}
			const int bx = (b->cell & (BricksPerAxis - 1)) << BrickShift;
			const int by = ((b->cell >> BricksPerAxisShift) & (BricksPerAxis - 1)) << BrickShift;
			const int bz = (b->cell >> (2 * BricksPerAxisShift)) << BrickShift;
			func(bx, by, bz, *b);
		}
	}

	/**
	 * @brief Visit all stored voxels - brick by brick in allocation order, inside a brick in index order
	 * @param func Called with the local x, y, z coordinates and the voxel
	 */
	template<class FUNC>
	void visit(FUNC &&func) const {
		for (int i = 0; i < _poolUsed; ++i) {
			const Brick *brick = poolBrick(i);
			if (brick->voxels == nullptr || brick->count == 0) {
				continue;
			}
			const int bx = (brick->cell & (BricksPerAxis - 1)) << BrickShift;
			const int by = ((brick->cell >> BricksPerAxisShift) & (BricksPerAxis - 1)) << BrickShift;
			const int bz = (brick->cell >> (2 * BricksPerAxisShift)) << BrickShift;
			const bool dense = brick->dense();
			int packed = 0;
			for (int w = 0; w < BrickMaskWords; ++w) {
				uint64_t bits = brick->mask[w];
				while (bits != 0u) {
					const int bit = core::countTrailingZeros(bits);
					bits &= bits - 1u;
					const Voxel &v = dense ? brick->voxels[(w << 6) | bit] : brick->voxels[packed++];
					func(bx + (bit & 7), by + (bit >> 3), bz + w, v);
				}
			}
		}
	}

	/**
	 * @brief Return a mutable pointer to an occupied voxel inside a brick, or nullptr if not present.
	 */
	static inline Voxel *mutableVoxelIfPresent(Brick *b, int idx) {
		const uint64_t word = b->mask[idx >> 6];
		const uint64_t bit = (uint64_t)1u << (idx & 63);
		if ((word & bit) == 0u) {
			return nullptr;
		}
		return b->dense() ? (b->voxels + idx) : (b->voxels + b->prefix[idx >> 6] + popcount64(word & (bit - 1u)));
	}
};

} // namespace voxel
