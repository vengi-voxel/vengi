/**
 * @file
 */

#include "VolumeDataAccessor.h"
#include "RawVolume.h"
#include "private/LinearVolumeStorage.h"
#include "core/Common.h"
#include "core/StandardLib.h"

namespace voxel {

void VolumeDataAccessor::init(const RawVolume &volume, const Region &region) {
	// The argument may refer to this accessor's current region.
	const Region requested = region;
	if (_owned) {
		core_free(_voxels);
	}
	_voxels = nullptr;
	_owned = false;
	_region = Region::InvalidRegion;
	if (!requested.isValid() || !volume.valid()) {
		return;
	}
	_region = requested;

	// Zero-copy borrow of the full linear buffer when the region matches.
	if (_region == volume.region()) {
		_voxels = volume._storage->voxels();
		return;
	}

	size_t bytes = sizeof(Voxel);
	for (int axis = 0; axis < 3; ++axis) {
		const int dimension = _region.getDimensionsInVoxels()[axis];
		if (dimension <= 0 || bytes > SIZE_MAX / (size_t)dimension) {
			_region = Region::InvalidRegion;
			return;
		}
		bytes *= (size_t)dimension;
	}
	_voxels = (Voxel *)core_malloc(bytes);
	if (_voxels == nullptr) {
		_region = Region::InvalidRegion;
		return;
	}
	_owned = true;
	core_memset((void *)_voxels, 0, bytes);

	Region overlap = _region;
	if (!overlap.cropTo(volume.region())) {
		return;
	}

	const glm::ivec3 &mins = overlap.getLowerCorner();
	const glm::ivec3 &dimensions = overlap.getDimensionsInVoxels();
	const Voxel *src = volume._storage->voxels();
	for (int dz = 0; dz < dimensions.z; ++dz) {
		const int z = mins.z + dz;
		for (int dy = 0; dy < dimensions.y; ++dy) {
			const int y = mins.y + dy;
			core_memcpy((void *)(_voxels + _region.index(mins.x, y, z)),
						(const void *)(src + volume.region().index(mins.x, y, z)),
						(size_t)dimensions.x * sizeof(Voxel));
		}
	}
}

VolumeDataAccessor::VolumeDataAccessor(const RawVolume &volume) {
	init(volume, volume.region());
}

VolumeDataAccessor::VolumeDataAccessor(const RawVolume &volume, const Region &region) {
	init(volume, region);
}

VolumeDataAccessor::VolumeDataAccessor(VolumeDataAccessor &&other) noexcept {
	*this = core::move(other);
}

VolumeDataAccessor &VolumeDataAccessor::operator=(VolumeDataAccessor &&other) noexcept {
	if (this == &other) {
		return *this;
	}
	if (_owned) {
		core_free(_voxels);
	}
	_region = other._region;
	_voxels = other._voxels;
	_owned = other._owned;
	other._region = Region::InvalidRegion;
	other._voxels = nullptr;
	other._owned = false;
	return *this;
}

VolumeDataAccessor::~VolumeDataAccessor() {
	if (_owned) {
		core_free(_voxels);
	}
}

} // namespace voxel
