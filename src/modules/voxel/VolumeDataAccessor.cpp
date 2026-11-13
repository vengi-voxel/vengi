/**
 * @file
 */

#include "VolumeDataAccessor.h"
#include "private/ChunkedVolumeStorage.h"
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
	const Region &copyRegion = _region;
	if (volume.isContiguous() && copyRegion == volume.region()) {
		_voxels = volume._data;
		return;
	}
	size_t bytes = sizeof(Voxel);
	for (int axis = 0; axis < 3; ++axis) {
		const int dimension = copyRegion.getDimensionsInVoxels()[axis];
		if (dimension <= 0 || bytes > SIZE_MAX / (size_t)dimension) {
			_region = Region::InvalidRegion;
			return;
		}
		bytes *= (size_t)dimension;
	}
	_voxels = (Voxel *)core_malloc(bytes);
	if (!_voxels) {
		_region = Region::InvalidRegion;
		return;
	}
	_owned = true;
	core_memset((void *)_voxels, 0, bytes);
	Region overlap = copyRegion;
	if (!overlap.cropTo(volume.region())) {
		return;
	}
	const glm::ivec3 &mins = overlap.getLowerCorner();
	const glm::ivec3 &dimensions = overlap.getDimensionsInVoxels();
	for (int dz = 0; dz < dimensions.z; ++dz) {
		const int z = mins.z + dz;
		for (int dy = 0; dy < dimensions.y; ++dy) {
			const int y = mins.y + dy;
			Voxel *destination = _voxels + copyRegion.index(mins.x, y, z);
			if (volume.isContiguous()) {
				core_memcpy((void *)destination, volume._data + volume.region().index(mins.x, y, z),
					(size_t)dimensions.x * sizeof(Voxel));
				continue;
			}
			// Copy X spans within each chunk, rather than dispatching per voxel.
			for (int dx = 0; dx < dimensions.x;) {
				const int x = mins.x + dx;
				int count;
				const Voxel *source = volume._storage->row(glm::ivec3(x, y, z) - volume.region().getLowerCorner(), count);
				count = core_min(count, dimensions.x - dx);
				if (source) {
					core_memcpy((void *)destination, source, count * sizeof(Voxel));
				} else {
					uint32_t pattern;
					static_assert(sizeof(pattern) == sizeof(Voxel), "Expected a four-byte voxel");
					core_memcpy(&pattern, &volume._storage->defaultVoxel(), sizeof(pattern));
					core_memset4(destination, pattern, count);
				}
				destination += count;
				dx += count;
			}
		}
	}
}

VolumeDataAccessor::VolumeDataAccessor(const RawVolume &volume) { init(volume, volume.region()); }
VolumeDataAccessor::VolumeDataAccessor(const RawVolume &volume, const Region &region) { init(volume, region); }
VolumeDataAccessor::VolumeDataAccessor(VolumeDataAccessor &&other) noexcept {
	*this = core::move(other);
}

VolumeDataAccessor &VolumeDataAccessor::operator=(VolumeDataAccessor &&other) noexcept {
	if (this != &other) {
		if (_owned) {
			core_free(_voxels);
		}
		_region = other._region;
		_voxels = other._voxels;
		_owned = other._owned;
		other._region = Region::InvalidRegion;
		other._voxels = nullptr;
		other._owned = false;
	}
	return *this;
}

VolumeDataAccessor::~VolumeDataAccessor() {
	if (_owned) {
		core_free(_voxels);
	}
}

} // namespace voxel
