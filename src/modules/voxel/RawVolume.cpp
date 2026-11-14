/**
 * @file
 */

#include "RawVolume.h"
#include "private/LinearVolumeStorage.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include <glm/common.hpp>

namespace voxel {

size_t RawVolume::size(const Region &region) {
	if (!region.isValid()) {
		return 0;
	}
	const size_t w = region.getWidthInVoxels();
	const size_t h = region.getHeightInVoxels();
	const size_t d = region.getDepthInVoxels();
	const size_t size = w * h * d * sizeof(Voxel);
	return size;
}

RawVolume::RawVolume(const Region &regValid) : _region(regValid) {
	// Create a volume of the right size.
	initialise(regValid);
}

RawVolume::RawVolume(const RawVolume *copy) : _region(copy->region()) {
	setBorderValue(copy->borderValue());
	_borderVoxel = copy->_borderVoxel;
	if (!copy->valid()) {
		initialise(_region);
		return;
	}
	_storage = new LinearVolumeStorage(*copy->_storage);
}

RawVolume::RawVolume(const RawVolume &copy) : _region(copy.region()) {
	setBorderValue(copy.borderValue());
	_borderVoxel = copy._borderVoxel;
	if (!copy.valid()) {
		initialise(_region);
		return;
	}
	_storage = new LinearVolumeStorage(*copy._storage);
}

static inline voxel::Region accumulate(const core::Buffer<Region> &regions) {
	voxel::Region r = voxel::Region::InvalidRegion;
	for (const Region &region : regions) {
		if (r.isValid()) {
			r.accumulate(region);
		} else {
			r = region;
		}
	}
	return r;
}

RawVolume::RawVolume(const RawVolume &src, const core::Buffer<Region> &copyRegions)
	: _region(accumulate(copyRegions)) {
	_region.cropTo(src.region());
	setBorderValue(src.borderValue());
	initialise(_region);

	for (const Region &copyRegion : copyRegions) {
		copyInto(src, copyRegion);
	}
}

RawVolume::RawVolume(const RawVolume& src, const Region& region, bool *onlyAir) : _region(region) {
	core_trace_scoped(RawVolumeCopyRegion);
	core_assert(region.isValid());
	setBorderValue(src.borderValue());
	if (!intersects(src.region(), _region)) {
		if (onlyAir) {
			*onlyAir = true;
		}
		initialise(_region);
	} else if (src.region() == _region) {
		if (!src.valid()) {
			initialise(_region);
			return;
		}
		_storage = new LinearVolumeStorage(*src._storage);
		if (onlyAir) {
			*onlyAir = false;
		}
	} else {
		if (!src.region().containsRegion(_region)) {
			_region.cropTo(src._region);
		}
		if (onlyAir) {
			*onlyAir = true;
		}
		// Destination is fully overwritten by the box copy below.
		initialise(_region, false);
		if (!valid()) {
			return;
		}
		const glm::ivec3 srcLocal = _region.getLowerCorner() - src._region.getLowerCorner();
		_storage->copyFrom(*src._storage, srcLocal, glm::ivec3(0), _region.getDimensionsInVoxels());
		if (onlyAir) {
			*onlyAir = _storage->isEmpty(glm::ivec3(0), _region.getDimensionsInVoxels() - 1);
		}
	}
}

static bool cropToLocal(const Region &volumeRegion, const Region &region, glm::ivec3 &localMins, glm::ivec3 &localMaxs) {
	if (!intersects(volumeRegion, region)) {
		return false;
	}
	Region r = region;
	if (!volumeRegion.containsRegion(r)) {
		r.cropTo(volumeRegion);
	}
	localMins = r.getLowerCorner() - volumeRegion.getLowerCorner();
	localMaxs = r.getUpperCorner() - volumeRegion.getLowerCorner();
	return true;
}

bool RawVolume::hasFlags(const Region &region, uint8_t flags) const {
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!cropToLocal(_region, region, localMins, localMaxs)) {
		return false;
	}
	return _storage->hasFlags(localMins, localMaxs, flags);
}

Region RawVolume::regionForFlag(uint8_t flag) const {
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!_storage->regionForFlag(flag, localMins, localMaxs)) {
		return Region::InvalidRegion;
	}
	const glm::ivec3 &lower = _region.getLowerCorner();
	return Region(lower + localMins, lower + localMaxs);
}

void RawVolume::removeFlags(const Region &region, uint8_t flags) {
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!cropToLocal(_region, region, localMins, localMaxs)) {
		return;
	}
	_storage->removeFlags(localMins, localMaxs, flags);
}

void RawVolume::toggleFlags(const Region &region, uint8_t flags) {
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!cropToLocal(_region, region, localMins, localMaxs)) {
		return;
	}
	_storage->toggleFlags(localMins, localMaxs, flags);
}

void RawVolume::setFlags(const Region &region, uint8_t flags) {
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!cropToLocal(_region, region, localMins, localMaxs)) {
		return;
	}
	_storage->setFlags(localMins, localMaxs, flags);
}

bool RawVolume::isEmpty(const Region &region) const {
	core_trace_scoped(RawVolumeIsEmpty);
	glm::ivec3 localMins;
	glm::ivec3 localMaxs;
	if (!cropToLocal(_region, region, localMins, localMaxs)) {
		return true;
	}
	return _storage->isEmpty(localMins, localMaxs);
}

bool RawVolume::copyInto(const RawVolume &src) {
	if (_region == src.region()) {
		_storage->copyFrom(*src._storage);
		return true;
	}
	voxel::Region srcRegion = src.region();
	return copyInto(src, srcRegion);
}

bool RawVolume::copyInto(const RawVolume &src, const voxel::Region &region) {
	core_trace_scoped(RawVolumeCopyInto);
	if (!intersects(_region, region)) {
		return false;
	}
	voxel::Region srcRegion = region;
	if (!srcRegion.cropTo(src.region())) {
		return false;
	}
	if (!srcRegion.cropTo(_region)) {
		return false;
	}
	const glm::ivec3 srcLocal = srcRegion.getLowerCorner() - src.region().getLowerCorner();
	const glm::ivec3 dstLocal = srcRegion.getLowerCorner() - _region.getLowerCorner();
	_storage->copyFrom(*src._storage, srcLocal, dstLocal, srcRegion.getDimensionsInVoxels());
	return true;
}

RawVolume::RawVolume(RawVolume &&move) noexcept {
	_storage = move._storage;
	move._storage = new LinearVolumeStorage(glm::ivec3(1));
	_region = move._region;
	_borderVoxel = move._borderVoxel;
}

RawVolume &RawVolume::operator=(RawVolume &&move) noexcept {
	if (this == &move) {
		return *this;
	}
	delete _storage;
	_storage = move._storage;
	move._storage = new LinearVolumeStorage(glm::ivec3(1));
	_region = move._region;
	_borderVoxel = move._borderVoxel;
	return *this;
}

RawVolume::RawVolume(const Voxel *data, const voxel::Region &region) {
	initialise(region);
	_storage->copyFrom(data);
}

RawVolume::RawVolume(Voxel *data, const voxel::Region &region) : _region(region) {
	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");
	_storage = new LinearVolumeStorage(data, {width(), height(), depth()}, true);
}

RawVolume::~RawVolume() {
	delete _storage;
}

bool RawVolume::move(const glm::ivec3 &shift) {
	if (!valid()) {
		return false;
	}
	return _storage->move(shift);
}

Voxel *RawVolume::copyVoxels() const {
	const size_t size = RawVolume::size(_region);
	Voxel *rawCopy = (Voxel *)core_malloc(size);
	if (rawCopy == nullptr || !valid()) {
		return rawCopy;
	}
	_storage->copyTo(rawCopy);
	return rawCopy;
}


/**
 * This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param x The @c x position of the voxel
 * @param y The @c y position of the voxel
 * @param z The @c z position of the voxel
 * @return The voxel value
 */
void RawVolume::fillBehindPlane(math::Axis axis, int slice, int minA, int maxA, int minB, int maxB, Voxel *out) const {
	if (out == nullptr || minA > maxA || minB > maxB ||
		(axis != math::Axis::X && axis != math::Axis::Y && axis != math::Axis::Z)) {
		return;
	}
	const int dAxis = math::getIndexForAxis(axis);
	const int aAxis = math::getIndexForAxis(math::getPlaneAxisA(axis));
	const int bAxis = math::getIndexForAxis(math::getPlaneAxisB(axis));
	glm::ivec3 lo(0);
	glm::ivec3 hi(0);
	lo[dAxis] = hi[dAxis] = slice;
	lo[aAxis] = minA;
	hi[aAxis] = maxA;
	lo[bAxis] = minB;
	hi[bAxis] = maxB;
	if (!_region.containsRegion(Region(lo, hi))) {
		return;
	}

	const glm::ivec3 &lower = _region.getLowerCorner();
	_storage->fillBehindPlane(dAxis, aAxis, bAxis, slice - lower[dAxis], minA - lower[aAxis], maxA - lower[aAxis],
							  minB - lower[bAxis], maxB - lower[bAxis], out);
}

/**
 * @param[in] voxel The value to use for voxels outside the volume.
 */
void RawVolume::setBorderValue(const Voxel &voxel) {
	_borderVoxel = voxel;
}

/**
 * This function should probably be made internal...
 */
void RawVolume::initialise(const Region &regValidRegion, bool clear) {
	_region = regValidRegion;

	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");

	delete _storage;
	_storage = new LinearVolumeStorage({width(), height(), depth()}, clear);
}

void RawVolume::clear() {
	Voxel voxel;
	fill(voxel);
}

void RawVolume::fill(const voxel::Voxel &voxel) {
	if (!valid()) {
		return;
	}
	_storage->fill(voxel);
}

bool RawVolume::valid() const {
	return _storage->width() == width() && _storage->height() == height() && _storage->depth() == depth();
}

size_t RawVolume::allocatedBytes() const {
	return valid() ? _storage->allocatedBytes() : 0;
}

} // namespace voxel
