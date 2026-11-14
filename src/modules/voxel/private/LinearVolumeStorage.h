/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include <glm/vec3.hpp>
#include <stddef.h>
#include <stdint.h>

namespace voxel {

/**
 * @brief Contiguous voxel storage addressed in volume-local coordinates.
 *
 * Layout is x fastest, then y, then z. The buffer is allocated for the full
 * dimensions unless an external buffer is adopted. Coordinates are relative to
 * the volume lower corner (0 .. dimensions-1). Concurrent writes to the same
 * voxel, fill, move, copy and destruction require external synchronization.
 */
class LinearVolumeStorage {
public:
	explicit LinearVolumeStorage(const glm::ivec3 &dimensions);
	/**
	 * @brief Adopt an existing contiguous buffer
	 * @param data Buffer of width*height*depth voxels; ownership is taken when @p owned is true
	 */
	LinearVolumeStorage(Voxel *data, const glm::ivec3 &dimensions, bool owned = true);
	LinearVolumeStorage(const LinearVolumeStorage &other);
	LinearVolumeStorage(LinearVolumeStorage &&other) noexcept;
	LinearVolumeStorage &operator=(const LinearVolumeStorage &) = delete;
	LinearVolumeStorage &operator=(LinearVolumeStorage &&other) noexcept;
	~LinearVolumeStorage();

	bool valid() const;
	const Voxel &voxel(const glm::ivec3 &localPos) const;
	bool setVoxel(const glm::ivec3 &localPos, const Voxel &value);
	Voxel *writableVoxel(const glm::ivec3 &localPos);
	/** Read a contiguous X span starting at @p localPos; always returns a real pointer when valid. */
	const Voxel *row(const glm::ivec3 &localPos, int &count) const;
	Voxel *sampleData(const glm::ivec3 &localPos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper, int64_t &yStride,
					  int64_t &zStride) const;
	void fill(const Voxel &value);
	/** Cyclic left shift; requires exclusive access. */
	bool move(const glm::ivec3 &shift);
	void copyTo(Voxel *destination) const;
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
	glm::ivec3 _dimensions{0};
	Voxel *_data = nullptr;
	bool _owned = true;

	size_t byteSize() const;
};

} // namespace voxel
