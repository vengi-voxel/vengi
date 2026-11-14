/**
 * @file
 */

#pragma once

#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stdint.h>

namespace voxel {

class RawVolume;

/**
 * @brief Contiguous voxel view for a volume region
 *
 * When the requested region matches the full volume, this is a zero-copy view
 * into the existing buffer. Otherwise a dense buffer is allocated for @p region
 * and filled from the volume (positions outside the volume are air). Writes to
 * owned snapshots do not update the source.
 */
class VolumeDataAccessor {
public:
	explicit VolumeDataAccessor(const RawVolume &volume);
	VolumeDataAccessor(const RawVolume &volume, const Region &region);

	VolumeDataAccessor(const VolumeDataAccessor &) = delete;
	VolumeDataAccessor &operator=(const VolumeDataAccessor &) = delete;

	VolumeDataAccessor(VolumeDataAccessor &&other) noexcept;
	VolumeDataAccessor &operator=(VolumeDataAccessor &&other) noexcept;

	~VolumeDataAccessor();

	/** Replace the current view, releasing any owned snapshot first. */
	void init(const RawVolume &volume, const Region &region);

	Voxel *voxels() {
		return _voxels;
	}
	const Voxel *voxels() const {
		return _voxels;
	}
	const uint8_t *data() const {
		return (const uint8_t *)_voxels;
	}
	const Region &region() const {
		return _region;
	}
	int32_t width() const {
		return _region.getWidthInVoxels();
	}
	int32_t height() const {
		return _region.getHeightInVoxels();
	}
	int32_t depth() const {
		return _region.getDepthInVoxels();
	}
	int64_t stride() const {
		return _region.stride();
	}
	bool ownsBuffer() const {
		return _owned;
	}
	bool valid() const {
		return _voxels != nullptr;
	}

private:
	Region _region = Region::InvalidRegion;
	Voxel *_voxels = nullptr;
	bool _owned = false;
};

} // namespace voxel
