/**
 * @file
 */

#pragma once

#include "Region.h"
#include "SparseChunk.h"
#include "VolumeSampler.h"
#include "Voxel.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include <glm/ext/vector_int3.hpp>

namespace voxel {

class SparseRawVolumeStorage;

/**
 * @brief Bounded voxel volume with contiguous array storage for small volumes and SparseChunk-backed storage
 * for large volumes.
 *
 * Regions below the configured voxel-count threshold use a contiguous flat Voxel array. Larger regions use a
 * fixed 3D directory of lazily allocated 256^3 @c SparseChunk instances (each subdividing into 32^3 bricks of
 * 8^3 voxels that stay packed for up to 64 voxels and promote to dense 512-voxel bricks above that).
 */
class RawVolume {
private:
	friend class RawVolumeView;
	template<typename Volume, bool DenseOnly>
	friend class VolumeSampler;

public:
	class Sampler : public VolumeSampler<RawVolume> {
	private:
		using Super = VolumeSampler<RawVolume>;

	public:
		VOLUMESAMPLERUSING;

		CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const Voxel &voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			if (core_likely(_currentVoxel != nullptr)) {
				*_currentVoxel = voxel;
				return true;
			}
			writeCurrentSparse(voxel, false);
			return true;
		}
	};

	/**
	 * Dense traversal for a full contiguous volume. Requires isContiguous().
	 * Prefer withVolumeSampler() to select this path without materializing sparse chunks.
	 */
	class DenseSampler : public VolumeSampler<RawVolume, true> {
		using Super = VolumeSampler<RawVolume, true>;

	public:
		using Super::VolumeSampler;

		CORE_FORCE_INLINE bool setVoxel(const Voxel &voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			*_currentVoxel = voxel;
			return true;
		}
	};

	RawVolume(const Voxel *data, const voxel::Region &region);
	RawVolume(Voxel *data, const voxel::Region &region);

public:
	/// Constructor for creating a fixed size volume.
	RawVolume(const Region &region);
	RawVolume(const RawVolume *copy);
	RawVolume(const RawVolume &copy);
	RawVolume(RawVolume &&move) noexcept;
	RawVolume(const RawVolume &copy, const Region &region, bool *onlyAir = nullptr);
	RawVolume(const RawVolume &copy, const core::Buffer<Region> &regions);

	/**
	 * @brief Set the cubic side length at which new volumes use SparseChunk storage. Must be positive.
	 * Existing volumes keep their storage.
	 */
	static void setChunkedStorageThreshold(int sideLength);
	static int chunkedStorageThreshold();

	bool copyInto(const RawVolume &src);
	bool copyInto(const RawVolume &src, const voxel::Region &region);

	/**
	 * @brief Calculate the amount of bytes a volume with the given region would consume
	 * @return Zero if the region is invalid or its byte count cannot fit in size_t.
	 */
	static size_t size(const Region &region);

	/**
	 * @brief Checks if the volume is empty in the given region
	 */
	bool isEmpty(const Region &region) const;

	/**
	 * @brief Compares region and raw voxel payload without materializing sparse storage
	 */
	bool equals(const RawVolume &other) const;

	/**
	 * @brief Compute bounds of all non-air voxels without materializing sparse storage
	 */
	bool computeSolidBounds(glm::ivec3 &mins, glm::ivec3 &maxs) const;

	/**
	 * @brief Read a sub-region into a tightly packed X-major buffer without materializing sparse storage
	 */
	void readRegion(const Region &region, Voxel *target) const;

	/**
	 * @brief Find the nearest non-air voxel behind a plane rectangle.
	 */
	void fillBehindPlane(math::Axis axis, int slice, int minA, int maxA, int minB, int maxB, Voxel *out) const;

	static RawVolume *createRaw(const Voxel *data, const voxel::Region &region) {
		return new RawVolume(data, region);
	}

	static RawVolume *createRaw(Voxel *data, const voxel::Region &region) {
		return new RawVolume(data, region);
	}

	~RawVolume();

	/**
	 * Copy the raw data of the volume
	 * @note It's the callers responsibility to properly release the memory.
	 */
	Voxel *copyVoxels() const;

	const Voxel &borderValue() const;

	const Region &region() const;

	Region &region() {
		return _region;
	}

	inline const Region &volumeRegion() const {
		return _region;
	}

	int32_t width() const;
	int32_t height() const;
	int32_t depth() const;

	glm::ivec3 mins() const;
	glm::ivec3 maxs() const;

	const Voxel &voxel(int32_t x, int32_t y, int32_t z) const;
	CORE_NO_SANITIZE_ADDRESS inline const Voxel &voxel(const glm::ivec3 &pos) const;

	void setBorderValue(const Voxel &voxel);
	CORE_NO_SANITIZE_ADDRESS bool setVoxel(int32_t x, int32_t y, int32_t z, const Voxel &voxel);
	CORE_NO_SANITIZE_ADDRESS bool setVoxel(const glm::ivec3 &pos, const Voxel &voxel);
	CORE_NO_SANITIZE_ADDRESS bool setVoxel(int64_t idx, const Voxel &voxel);
	CORE_NO_SANITIZE_ADDRESS bool setVoxelOverwrite(int32_t x, int32_t y, int32_t z, const Voxel &voxel);

	/** Batch write of a single voxel value along +X starting at (x, y, z). */
	void setVoxelsRow(int32_t x, int32_t y, int32_t z, int32_t count, const Voxel &voxel);

	void clear();
	void fill(const voxel::Voxel &voxel);

	/**
	 * Obtain contiguous storage, materializing sparse chunks if needed.
	 */
	inline const uint8_t *data() const {
		return (const uint8_t *)voxels();
	}

	Voxel *voxels() const;

	/** Existing contiguous buffer, or nullptr for sparse storage; does not materialize. */
	inline Voxel *linearVoxels() const {
		return _data;
	}

	inline bool isContiguous() const {
		return _storage == nullptr && _data != nullptr;
	}

	/**
	 * @brief Pre-packed or dense brick produced by a worker thread for pointer-transfer adoption.
	 */
	struct PreparedTileBrick {
		size_t chunkIndex = 0u;
		SparseChunk::PreparedBrick brick;
	};

	inline bool isSparse() const {
		return _storage != nullptr;
	}

	/** True when backed by SparseChunk storage and default voxel material is Air. */
	bool isSparseAirDefault() const;
	Voxel sparseDefaultVoxel() const;
	/** O(1) brick/chunk occupancy test for skipping empty source tiles. */
	bool hasOccupiedVoxelsInRegion(const Region &region) const;
	/** Lock-free lookup of the brick covering volume-local coordinate (localX, localY, localZ), or nullptr. */
	const SparseChunk::Brick *sparseBrickAt(int localX, int localY, int localZ) const;
	bool readSparseBrickTile(const glm::ivec3 &localBrickOrigin, Voxel (&outTile)[SparseChunk::BrickVoxels],
							 uint64_t (&outMask)[SparseChunk::BrickMaskWords], int &outCount) const;
	size_t sparseChunkIndex(const glm::ivec3 &localBrickOrigin) const;
	void adoptPreparedSparseBricks(core::DynamicArray<PreparedTileBrick> &prepared);

	const uint32_t *sparseGenerationPtr() const;

	void sampleSparseAt(int lx, int ly, int lz, const SparseChunk *&outChunk, const SparseChunk::Brick *&outBrick,
						const Voxel *&outRow, bool &outWholeBrick, glm::ivec3 &outLower, glm::ivec3 &outUpper,
						Voxel &outFallbackVoxel, uint32_t &outGen) const;

	/** Allocated payload and directory bytes, excluding allocator overhead. */
	size_t allocatedBytes() const;

	/** True if the volume has allocated storage. */
	bool valid() const;

	void translate(const glm::ivec3 &t) {
		_region.shift(t.x, t.y, t.z);
	}

	bool move(const glm::ivec3 &t);

	void removeFlags(const Region &region, uint8_t flags);
	void setFlags(const Region &region, uint8_t flags);
	bool hasFlags(const Region &region, uint8_t flags) const;
	void toggleFlags(const Region &region, uint8_t flags);
	Region regionForFlag(uint8_t flag) const;

private:
	void initialise(const Region &region, bool forceDense = false);

	/** The size of the volume */
	Region _region;

	/** The border value */
	Voxel _borderVoxel;

	/** The voxel data (contiguous mode) */
	mutable Voxel *_data = nullptr;

	/** SparseChunk-backed storage (large volume mode) */
	mutable SparseRawVolumeStorage *_storage = nullptr;
};

inline const Region &RawVolume::region() const {
	return _region;
}

inline const Voxel &RawVolume::borderValue() const {
	return _borderVoxel;
}

inline int32_t RawVolume::width() const {
	return _region.getWidthInVoxels();
}

inline int32_t RawVolume::height() const {
	return _region.getHeightInVoxels();
}

inline int32_t RawVolume::depth() const {
	return _region.getDepthInVoxels();
}

inline const Voxel &RawVolume::voxel(const glm::ivec3 &pos) const {
	return voxel(pos.x, pos.y, pos.z);
}

} // namespace voxel
