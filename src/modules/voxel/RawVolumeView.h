/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

/**
 * @brief A view into a raw volume
 * @ingroup Voxel
 *
 * This class is used to access a raw volume in a specific region. It provides
 * an interface to access the voxels in that region without copying the data.
 * The view is read-only and does not modify the original volume.
 * The view is created with a specific region, and the voxels can be accessed
 * using the [] operator.
 */
class RawVolumeView {
protected:
	const RawVolume *_volume;
	const Region _region;

public:
	RawVolumeView(const RawVolume *volume, const Region &region) : _volume(volume), _region(region) {
	}

	/**
	 * @param[in] x The x position of the voxel relative to the view region
	 * @param[in] y The y position of the voxel relative to the view region
	 * @param[in] z The z position of the voxel relative to the view region
	 */
	inline const Voxel &voxel(int x, int y, int z) const {
		return voxel({x, y, z});
	}

	/**
	 * @param[in] pos The position of the voxel relative to the view region
	 */
	inline const Voxel &voxel(const glm::ivec3 &pos) const {
		const glm::ivec3 volumePos = pos + _region.getLowerCorner();
		if (!_region.containsPoint(volumePos)) {
			return _volume->borderValue();
		}
		return _volume->voxel(volumePos);
	}

	inline glm::ivec3 viewPosFromIndex(size_t idx) const {
		const int width = _region.getWidthInVoxels();
		const int height = _region.getHeightInVoxels();
		const int dim = width * height;
		const int z = idx / dim;
		const int y = (idx % dim) / width;
		const int x = idx % width;
		return {x, y, z};
	}

	/**
	 * @brief Operator to access voxels with x running fastest, followed by y and last z
	 * @see viewPosFromIndex()
	 */
	inline const Voxel &operator[](size_t idx) const {
		if ((int)idx >= _region.voxels()) {
			return _volume->borderValue();
		}
		return _volume->voxel(viewPosFromIndex(idx) + _region.getLowerCorner());
	}

	inline const RawVolume *volume() const {
		return _volume;
	}

	inline const Region &region() const {
		return _region;
	}
};

} // namespace voxel
