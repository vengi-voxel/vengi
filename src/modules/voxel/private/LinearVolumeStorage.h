/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <stddef.h>
#include <stdint.h>

namespace voxel {

/**
 * @brief Contiguous voxel storage addressed in volume-local coordinates.
 *
 * Layout is x fastest, then y, then z. The buffer is always allocated for the
 * full dimensions (or adopted). Coordinates are relative to the volume lower
 * corner (0 .. dimensions-1). Concurrent writes to the same voxel, fill, move,
 * copy and destruction require external synchronization.
 */
class LinearVolumeStorage {
public:
	/**
	 * @param clear If true, fill the buffer with air voxels after allocation
	 */
	explicit LinearVolumeStorage(const glm::ivec3 &dimensions, bool clear = true);
	/**
	 * @brief Adopt an existing contiguous buffer
	 * @param data Buffer of width*height*depth voxels; ownership is taken when @p owned is true
	 */
	LinearVolumeStorage(Voxel *data, const glm::ivec3 &dimensions, bool owned = true);
	LinearVolumeStorage(const LinearVolumeStorage &other);
	LinearVolumeStorage(LinearVolumeStorage &&other) noexcept;
	LinearVolumeStorage &operator=(const LinearVolumeStorage &other);
	LinearVolumeStorage &operator=(LinearVolumeStorage &&other) noexcept;
	~LinearVolumeStorage();

	CORE_FORCE_INLINE const Voxel &voxel(const glm::ivec3 &localPos) const {
		return _data[index(localPos)];
	}
	CORE_FORCE_INLINE bool setVoxel(const glm::ivec3 &localPos, const Voxel &value) {
		Voxel &target = _data[index(localPos)];
		if (target == value) {
			return false;
		}
		target = value;
		return true;
	}
	CORE_FORCE_INLINE bool setVoxel(int64_t idx, const Voxel &value) {
		const int64_t total = (int64_t)_dimensions.x * (int64_t)_dimensions.y * (int64_t)_dimensions.z;
		if (idx < 0 || idx >= total) {
			return false;
		}
		if (_data[idx] == value) {
			return false;
		}
		_data[idx] = value;
		return true;
	}
	CORE_FORCE_INLINE Voxel *writableVoxel(const glm::ivec3 &localPos) {
		return _data + index(localPos);
	}
	/** Read a contiguous X span starting at @p localPos. */
	CORE_FORCE_INLINE const Voxel *row(const glm::ivec3 &localPos, int &count) const {
		count = _dimensions.x - localPos.x;
		return _data + index(localPos);
	}
	/**
	 * @brief Contiguous span covering @p localPos for sampler walks.
	 *
	 * Returns a pointer to the voxel at @p localPos and the inclusive local AABB
	 * plus element strides of that buffer. A fully linear backend reports the
	 * whole volume; a chunked backend reports the loaded chunk so VolumeSampler
	 * can walk inside it and re-bind when leaving the AABB.
	 */
	CORE_FORCE_INLINE Voxel *sampleData(const glm::ivec3 &localPos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper,
										int64_t &yStride, int64_t &zStride) const {
		dataLower = glm::ivec3(0);
		dataUpper = _dimensions - 1;
		yStride = _dimensions.x;
		zStride = stride();
		return _data + index(localPos);
	}
	void fill(const Voxel &value);
	/** Cyclic left shift; requires exclusive access. */
	bool move(const glm::ivec3 &shift);
	void copyTo(Voxel *destination) const;
	/** Full-volume copy; dimensions must match. */
	void copyFrom(const LinearVolumeStorage &src);
	/** Copy a dense box; coordinates and @p size are storage-local. */
	void copyFrom(const LinearVolumeStorage &src, const glm::ivec3 &srcMins, const glm::ivec3 &dstMins,
				  const glm::ivec3 &size);
	/** Replace the full buffer from a contiguous source of matching size. */
	void copyFrom(const Voxel *src);

	bool hasFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) const;
	/**
	 * @brief Tight local AABB of voxels that carry @p flag.
	 * @return @c false if no voxel carries the flag
	 */
	bool regionForFlag(uint8_t flag, glm::ivec3 &outLocalMins, glm::ivec3 &outLocalMaxs) const;
	void setFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags);
	void removeFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags);
	void toggleFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags);
	bool isEmpty(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs) const;
	/**
	 * @brief Walk behind a local plane and fill air slots in @p out.
	 *
	 * @p localSlice/@p minA/@p maxA/@p minB/@p maxB are inclusive storage-local
	 * coordinates on the given axis indices (0=X, 1=Y, 2=Z).
	 */
	void fillBehindPlane(int dAxis, int aAxis, int bAxis, int localSlice, int minA, int maxA, int minB, int maxB,
						 Voxel *out) const;

	/** Allocated voxel payload bytes, excluding allocator overhead. */
	size_t allocatedBytes() const;

	Voxel *voxels() {
		return _data;
	}
	const Voxel *voxels() const {
		return _data;
	}

	const glm::ivec3 &dimensions() const {
		return _dimensions;
	}
	int32_t width() const {
		return _dimensions.x;
	}
	int32_t height() const {
		return _dimensions.y;
	}
	int32_t depth() const {
		return _dimensions.z;
	}
	int64_t stride() const {
		return (int64_t)_dimensions.x * (int64_t)_dimensions.y;
	}
	int64_t index(const glm::ivec3 &localPos) const {
		return (int64_t)localPos.x + (int64_t)localPos.y * (int64_t)_dimensions.x +
			   (int64_t)localPos.z * stride();
	}

private:
	enum class FlagOp {
		Remove = 0,
		Toggle = 1,
		Set = 2
	};

	glm::ivec3 _dimensions;
	Voxel *_data;
	bool _owned = true;

	size_t byteSize() const;
	void resetToPlaceholder();
	void applyFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags, FlagOp op);
};

} // namespace voxel
