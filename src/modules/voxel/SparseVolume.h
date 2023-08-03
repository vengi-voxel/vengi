/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/DynamicMap.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxel {

class SparseVolume {
private:
	core::DynamicMap<glm::ivec3, voxel::Voxel, 31, glm::hash<glm::ivec3>> _map;
	static const constexpr voxel::Voxel _emptyVoxel{VoxelType::Air, 0};
	const voxel::Region _region;
	const bool _isRegionValid;

public:
	SparseVolume(const voxel::Region &region = voxel::Region::InvalidRegion);

	[[nodiscard]] inline const voxel::Region &region() const {
		return _region;
	}

	inline bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
		return setVoxel({x, y, z}, voxel);
	}

	bool setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel);

	/**
	 * Gets a voxel at the position given by @c x,y,z coordinates
	 */
	[[nodiscard]] const Voxel &voxel(int32_t x, int32_t y, int32_t z) const {
		return voxel({x, y, z});
	}

	/**
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	[[nodiscard]] const Voxel &voxel(const glm::ivec3 &pos) const;

	[[nodiscard]] inline bool empty() const {
		return size() == 0;
	}

	[[nodiscard]] inline size_t size() const {
		return _map.size();
	}

	void copyTo(voxel::RawVolumeWrapper &target) const;

	void copyFrom(const voxel::RawVolume &source);
};

} // namespace voxel
