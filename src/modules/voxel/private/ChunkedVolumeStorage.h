/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include <glm/vec3.hpp>
#include <stddef.h>
#include "core/concurrent/Atomic.h"

namespace voxel {

/**
 * @brief Bounded, lazily allocated voxel storage, addressed in volume-local coordinates.
 *
 * The directory is fixed for the lifetime of the storage. Reads do not allocate
 * or update shared caches. Chunk publication uses core::AtomicPtr; chunks are never
 * evicted, so references into allocated chunks stay valid until destruction.
 * Concurrent writes to distinct voxels are supported, including in the same
 * newly allocated chunk. Writes to the same voxel, fill, move, copy and destruction
 * require external synchronization. References to missing cells refer to the
 * default value, not to a subsequently allocated cell.
 */
class ChunkedVolumeStorage {
public:
	static constexpr int ChunkSide = 32;

	explicit ChunkedVolumeStorage(const glm::ivec3 &dimensions);
	ChunkedVolumeStorage(const ChunkedVolumeStorage &other);
	ChunkedVolumeStorage &operator=(const ChunkedVolumeStorage &) = delete;
	~ChunkedVolumeStorage();

	bool valid() const;
	const Voxel &voxel(const glm::ivec3 &localPos) const;
	bool setVoxel(const glm::ivec3 &localPos, const Voxel &value);
	Voxel *writableVoxel(const glm::ivec3 &localPos);
	/** Read a contiguous X span; nullptr denotes count copies of defaultVoxel(). */
	const Voxel *row(const glm::ivec3 &localPos, int &count) const;
	Voxel *sampleData(const glm::ivec3 &localPos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper, int64_t &yStride, int64_t &zStride, core::AtomicPtr<Voxel> *&chunkEntry, const Voxel *&defaultVoxel) const;
	const Voxel &defaultVoxel() const { return _default; }
	void fill(const Voxel &value);
	/** Cyclic left shift; preserves chunk addresses and requires exclusive access. */
	bool move(const glm::ivec3 &shift);
	void copyTo(Voxel *destination) const;
	/** Allocated directory and voxel payload bytes, excluding allocator overhead. */
	size_t allocatedBytes() const;

private:
	glm::ivec3 _dimensions;
	glm::ivec3 _chunkDimensions;
	glm::ivec3 _chunkCounts;
	size_t _directorySize = 0;
	size_t _chunkVoxels = 0;
	core::AtomicPtr<Voxel> *_chunks = nullptr;
	Voxel _default;

	size_t chunkIndex(const glm::ivec3 &pos) const;
	size_t voxelIndex(const glm::ivec3 &pos) const;
	Voxel *chunk(size_t index) const;
	bool chunkIsDefault(const Voxel *data) const;
	Voxel *createChunk(size_t index);
};

} // namespace voxel
