/**
 * @file
 */

#pragma once

#include "core/collection/DynamicBitSet.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxel {

/**
 * @brief Stores only 1 bit per voxel (solid or air). This means that a volume of size 1024x1024x1024 still requires 128 MB of memory.
 * @ingroup Voxel
 */
class BitVolume {
private:
	core::DynamicBitSet _data;
	voxel::Region _region;

public:
	BitVolume(const voxel::Region &region) : _data(region.voxels()), _region(region) {
	}

	BitVolume& operator=(BitVolume&& other) noexcept {
		if (this != &other) {
			_data = core::move(other._data);
			_region = other._region;
		}
		return *this;
	}

	BitVolume(BitVolume&& other) noexcept : _data(core::move(other._data)), _region(other._region) {
	}

	inline bool hasValue(int x, int y, int z) const {
		if (!_region.containsPoint(x, y, z)) {
			return false;
		}
		const size_t idx = _region.index(x, y, z);
		return _data[idx];
	}

	/**
	 * @note This voxel volume only stores whether a voxel is set or not. Therefore, this method will return a voxel
	 * that doesn't carry any meaningful data except for being air or not. In other words, the color and flags of a
	 * @c voxel::Voxel instance you've given to this volume is lost.
	 */
	inline voxel::Voxel voxel(int x, int y, int z) const {
		if (!_region.containsPoint(x, y, z)) {
			return voxel::createVoxel(voxel::VoxelType::Air, 0);
		}
		const size_t idx = _region.index(x, y, z);
		static_assert((int)voxel::VoxelType::Air == 0, "VoxelType::Air must be 0");
		return voxel::createVoxel((voxel::VoxelType)_data[idx], 0);
	}

	inline void setVoxel(int x, int y, int z, const voxel::Voxel &value) {
		setVoxel(x, y, z, voxel::isBlocked(value.getMaterial()));
	}

	inline void setVoxel(int x, int y, int z, bool value) {
		if (!_region.containsPoint(x, y, z)) {
			return;
		}
		const size_t idx = _region.index(x, y, z);
		_data.set(idx, value);
	}

	inline void setVoxel(const glm::ivec3 &pos, const voxel::Voxel &value) {
		setVoxel(pos.x, pos.y, pos.z, value);
	}

	inline void setVoxel(const glm::ivec3 &pos, bool value) {
		setVoxel(pos.x, pos.y, pos.z, value);
	}

	/**
	 * @return A Region representing the extent of the volume.
	 */
	inline const Region &region() const {
		return _region;
	}

	/**
	 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the width is 64.
	 * @sa height(), getDepth()
	 */
	inline int32_t width() const {
		return _region.getWidthInVoxels();
	}

	/**
	 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the height is 64.
	 * @sa width(), getDepth()
	 */
	inline int32_t height() const {
		return _region.getHeightInVoxels();
	}

	/**
	 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the depth is 64.
	 * @sa width(), height()
	 */
	inline int32_t depth() const {
		return _region.getDepthInVoxels();
	}

	/**
	 * the vector that describes the mins value of an aabb where a voxel is set in this volume
	 * deleting a voxel afterwards might lead to invalid results
	 */
	inline glm::ivec3 mins() const {
		return _region.getLowerCorner();
	}

	/**
	 * the vector that describes the maxs value of an aabb where a voxel is set in this volume
	 * deleting a voxel afterwards might lead to invalid results
	 */
	inline glm::ivec3 maxs() const {
		return _region.getUpperCorner();
	}

	inline size_t bytes() const {
		return _data.bytes();
	}
};

} // namespace voxel