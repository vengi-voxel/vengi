/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "voxel/SparseChunk.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>
#include <stddef.h>
#include <stdint.h>

namespace voxel {

/**
 * @brief Bounded, lazily allocated voxel storage backed by a fixed 3D directory of 256^3 SparseChunk instances.
 *
 * Coordinates are volume-local in [0, dimensions). Untouched chunks and absent brick cells evaluate to
 * @c defaultVoxel() without allocating payload memory.
 *
 * Concurrency model:
 * - Concurrent reads on an immutable volume are completely lock-free and do not mutate shared state.
 * - Concurrent writes to distinct voxels are serialized per 256^3 chunk slot via a per-slot mutex.
 * - Bulk operations (fill, move, copy, destruction) require exclusive access.
 */
class SparseRawVolumeStorage {
public:
	static constexpr int ChunkShift = SparseChunk::SideShift;
	static constexpr int ChunkSide = SparseChunk::Side;
	static constexpr int ChunkMask = ChunkSide - 1;
	static constexpr int BrickShift = SparseChunk::BrickShift;
	static constexpr int BrickSide = SparseChunk::BrickSide;
	static constexpr int BrickMask = BrickSide - 1;

	explicit SparseRawVolumeStorage(const glm::ivec3 &dimensions);
	SparseRawVolumeStorage(const SparseRawVolumeStorage &other);
	SparseRawVolumeStorage &operator=(const SparseRawVolumeStorage &) = delete;
	~SparseRawVolumeStorage();

	bool valid() const {
		return _slots != nullptr && _directorySize > 0u;
	}

	const glm::ivec3 &dimensions() const {
		return _dimensions;
	}

	const Voxel &defaultVoxel() const {
		return _default;
	}

	bool defaultIsZero() const {
		return _defaultIsZero;
	}

	inline const core::AtomicInt *generationPtr() const {
		return &_generation;
	}

	/** Lock-free voxel lookup in volume-local coordinates. */
	const Voxel &voxel(int lx, int ly, int lz) const;

	/** Thread-safe voxel write with Voxel::operator= normal-merge semantics. */
	bool setVoxel(int lx, int ly, int lz, const Voxel &value);

	/** Thread-safe voxel write with full field overwrite semantics. */
	bool setVoxelOverwrite(int lx, int ly, int lz, const Voxel &value);

	/** Thread-safe batch row write of a single voxel value along +X. */
	void setVoxelsRow(int lx, int ly, int lz, int count, const Voxel &value);

	/** Lock-free read of a contiguous X-major box [localMins, localMins + size) into target. */
	void readRegion(const glm::ivec3 &localMins, const glm::ivec3 &size, Voxel *target) const;

	/** Lock-free full-volume copy into a contiguous X-major buffer. */
	void copyTo(Voxel *destination) const;

	/** Replace all voxels with @p value and release all allocated chunks. */
	void fill(const Voxel &value);

	/** True if every voxel in [localMins, localMaxs] has all 4 bytes equal to 0. */
	bool isEmpty(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs) const;

	/** True if any voxel in [localMins, localMaxs] has any bit of @p flags set. */
	bool hasFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) const;

	/** Compute local bounds of all voxels having any bit of @p flag set. */
	bool regionForFlag(uint8_t flag, glm::ivec3 &outMins, glm::ivec3 &outMaxs) const;

	/** Modify flags in [localMins, localMaxs]. operation: 0 = set, 1 = remove, 2 = toggle. */
	void modifyFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags, int operation);

	/** Compute local bounds of all non-air voxels without materializing. */
	bool computeSolidBounds(glm::ivec3 &outMins, glm::ivec3 &outMaxs) const;

	/** Compare two storages with identical dimensions without materializing. */
	bool equals(const SparseRawVolumeStorage &other) const;

	/** Compare this storage against a flat contiguous array of identical dimensions. */
	bool equalsDense(const Voxel *denseData) const;

	/** Cyclic left shift; requires exclusive access. */
	bool move(const glm::ivec3 &shift);

	/**
	 * @brief Borrow a brick or brick-row for fast sampler traversal (lock-free).
	 *
	 * On return:
	 * - @p outRow is non-null if a contiguous BrickSide-aligned X-row (or full dense/empty brick) can be indexed by
	 *   (localX & BrickMask). If @p outWholeBrick is true, @p outRow points inside a full 8^3 dense/empty brick so
	 *   Y/Z steps within [outLower, outUpper] can adjust @p outRow by BrickSide / BrickSide^2 directly.
	 * - If @p outRow is nullptr (partially-filled packed brick row), the caller uses @p outFallbackVoxel.
	 */
	void sampleAt(int lx, int ly, int lz, const SparseChunk *&outChunk, const SparseChunk::Brick *&outBrick,
				  const Voxel *&outRow, bool &outWholeBrick, glm::ivec3 &outLower, glm::ivec3 &outUpper,
				  Voxel &outFallbackVoxel, uint32_t &outGen) const;

	/** Allocated directory and SparseChunk bytes, excluding allocator overhead. */
	size_t allocatedBytes() const;

	struct PreparedChunkBrick {
		size_t chunkIndex = 0u;
		SparseChunk::PreparedBrick brick;
	};

	struct OccupiedBrickInfo {
		glm::ivec3 localOrigin{0};
		SparseChunk::Brick *brick = nullptr;
	};

	/** O(1) check if any brick (or non-zero default) exists in [localMins, localMaxs]. */
	bool hasOccupiedVoxelsInBox(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs) const;

	/** Lock-free lookup of the brick covering volume-local coordinate (lx, ly, lz), or nullptr. */
	const SparseChunk::Brick *brickAt(int lx, int ly, int lz) const;

	/** Lock-free read of an 8-aligned brick into a 512-voxel staging tile and mask (relative to _default). */
	bool readBrickTile(int lx, int ly, int lz, Voxel (&outTile)[SparseChunk::BrickVoxels],
					   uint64_t (&outMask)[SparseChunk::BrickMaskWords], int &outCount) const;

	/** Compute the chunk directory index for a volume-local coordinate. */
	inline size_t chunkIndexAt(int lx, int ly, int lz) const {
		return chunkIndexForLocal(lx, ly, lz);
	}

	/**
	 * @brief Adopt pre-built bricks from worker threads via O(1) pointer transfer (exclusive access).
	 * Empty bricks (count == 0) release existing bricks; empty chunks are cleared.
	 */
	void adoptPreparedBricks(core::DynamicArray<PreparedChunkBrick> &prepared);

	/**
	 * @brief Collect all live bricks intersecting [localMins, localMaxs] for lock-free parallel read or
	 * in-place value mutation.
	 */
	void collectOccupiedBricks(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs,
							   core::DynamicArray<OccupiedBrickInfo> &out) const;

	/** Bump generation counter after in-place or batch modifications. */
	void notifyModified() {
		bumpGeneration();
	}

	static inline uint32_t voxelBits(const Voxel &v) {
		uint32_t bits = 0u;
		memcpy(&bits, &v, sizeof(bits));
		return bits;
	}

private:
	struct ChunkSlot {
		core::AtomicPtr<SparseChunk> chunk{nullptr};
		mutable core_trace_mutex(core::Lock, lock, "SparseRawVolumeStorageChunk");
	};

	glm::ivec3 _dimensions{0};
	glm::ivec3 _chunkCounts{0};
	size_t _directorySize = 0u;
	ChunkSlot *_slots = nullptr;
	Voxel _default{};
	bool _defaultIsZero = true;
	core::AtomicInt _generation{1};

	inline void bumpGeneration() {
		++_generation;
	}

	inline size_t chunkIndex(int cx, int cy, int cz) const {
		return ((size_t)cz * (size_t)_chunkCounts.y + (size_t)cy) * (size_t)_chunkCounts.x + (size_t)cx;
	}

	inline size_t chunkIndexForLocal(int lx, int ly, int lz) const {
		return chunkIndex(lx >> ChunkShift, ly >> ChunkShift, lz >> ChunkShift);
	}

	SparseChunk *findOrCreateChunkLocked(ChunkSlot &slot);
	void releaseChunkIfEmptyLocked(ChunkSlot &slot, SparseChunk *chunk);
	bool setVoxelLocked(ChunkSlot &slot, int clx, int cly, int clz, const Voxel &value, bool overwrite);
};

} // namespace voxel
