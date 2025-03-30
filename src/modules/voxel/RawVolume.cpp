/**
 * @file
 */

#include "RawVolume.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include <glm/common.hpp>
#include <limits>

namespace voxel {

size_t RawVolume::size(const Region &region) {
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
	const size_t size = RawVolume::size(_region);
	_data = (Voxel *)core_malloc(size);
	_borderVoxel = copy->_borderVoxel;
	core_memcpy((void*)_data, (void*)copy->_data, size);
}

RawVolume::RawVolume(const RawVolume &copy) : _region(copy.region()) {
	setBorderValue(copy.borderValue());
	const size_t size = RawVolume::size(_region);
	_data = (Voxel *)core_malloc(size);
	_borderVoxel = copy._borderVoxel;
	core_memcpy((void*)_data, (void*)copy._data, size);
}

static inline voxel::Region accumulate(const core::DynamicArray<Region> &regions) {
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

RawVolume::RawVolume(const RawVolume &src, const core::DynamicArray<Region> &copyRegions)
	: _region(accumulate(copyRegions)) {
	_region.cropTo(src.region());
	setBorderValue(src.borderValue());
	initialise(_region);

	for (Region copyRegion : copyRegions) {
		core_assert(copyRegion.isValid());
		copyRegion.cropTo(_region);
		RawVolume::Sampler destSampler(*this);
		RawVolume::Sampler srcSampler(src);
		for (int32_t x = copyRegion.getLowerX(); x <= copyRegion.getUpperX(); ++x) {
			for (int32_t y = copyRegion.getLowerY(); y <= copyRegion.getUpperY(); ++y) {
				srcSampler.setPosition(x, y, copyRegion.getLowerZ());
				destSampler.setPosition(x, y, copyRegion.getLowerZ());
				for (int32_t z = copyRegion.getLowerZ(); z <= copyRegion.getUpperZ(); ++z) {
					destSampler.setVoxel(srcSampler.voxel());
					destSampler.movePositiveZ();
					srcSampler.movePositiveZ();
				}
			}
		}
	}
}

RawVolume::RawVolume(const RawVolume& src, const Region& region, bool *onlyAir) : _region(region) {
	core_assert(region.isValid());
	setBorderValue(src.borderValue());
	const size_t size = RawVolume::size(_region);
	_data = (Voxel *)core_malloc(size);
	if (!intersects(src.region(), _region)) {
		if (onlyAir) {
			*onlyAir = true;
		}
		core_memset((void *)_data, 0, size);
	} else if (src.region() == _region) {
		core_memcpy((void *)_data, (void *)src._data, size);
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
		const glm::ivec3 &tgtMins = _region.getLowerCorner();
		const glm::ivec3 &tgtMaxs = _region.getUpperCorner();
		const glm::ivec3 &srcMins = src._region.getLowerCorner();
		const int tgtYStride = _region.getWidthInVoxels();
		const int tgtZStride = _region.getWidthInVoxels() * _region.getHeightInVoxels();
		const int srcYStride = src._region.getWidthInVoxels();
		const int srcZStride = src._region.getWidthInVoxels() * src._region.getHeightInVoxels();
		for (int x = tgtMins.x; x <= tgtMaxs.x; ++x) {
			const int32_t tgtXPos = x - tgtMins.x;
			const int32_t srcXPos = x - srcMins.x;
			for (int y = tgtMins.y; y <= tgtMaxs.y; ++y) {
				const int32_t tgtYPos = y - tgtMins.y;
				const int32_t srcYPos = y - srcMins.y;
				const int tgtStrideLocal = tgtXPos + tgtYPos * tgtYStride;
				const int srcStrideLocal = srcXPos + srcYPos * srcYStride;
				for (int z = tgtMins.z; z <= tgtMaxs.z; ++z) {
					const int32_t tgtZPos = z - tgtMins.z;
					const int32_t srcZPos = z - srcMins.z;
					const int tgtindex = tgtStrideLocal + tgtZPos * tgtZStride;
					const int srcindex = srcStrideLocal + srcZPos * srcZStride;
					_data[tgtindex] = src._data[srcindex];
					if (onlyAir && !voxel::isAir(_data[tgtindex].getMaterial())) {
						*onlyAir = false;
						onlyAir = nullptr;
					}
				}
			}
		}
	}
}

void RawVolume::copyInto(const RawVolume &src) {
	if (!intersects(_region, src.region())) {
		return;
	}
	voxel::Region srcRegion = src.region();
	if (!_region.containsRegion(srcRegion)) {
		srcRegion.cropTo(_region);
	}
	const glm::ivec3 &mins = srcRegion.getLowerCorner();
	const glm::ivec3 &maxs = srcRegion.getUpperCorner();
	const int tgtYStride = _region.getWidthInVoxels();
	const int tgtZStride = _region.getWidthInVoxels() * _region.getHeightInVoxels();
	const int srcYStride = srcRegion.getWidthInVoxels();
	const int srcZStride = srcRegion.getWidthInVoxels() * srcRegion.getHeightInVoxels();
	for (int x = mins.x; x <= maxs.x; ++x) {
		const int32_t srcXPos = x - mins.x;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int32_t srcYPos = y - mins.y;
			const int tgtStrideLocal = srcXPos + srcYPos * tgtYStride;
			const int srcStrideLocal = srcXPos + srcYPos * srcYStride;
			for (int z = mins.z; z <= maxs.z; ++z) {
				const int32_t srcZPos = z - mins.z;
				const int tgtindex = tgtStrideLocal + srcZPos * tgtZStride;
				const int srcindex = srcStrideLocal + srcZPos * srcZStride;
				_data[tgtindex] = src._data[srcindex];
			}
		}
	}
}

RawVolume::RawVolume(RawVolume &&move) noexcept {
	_data = move._data;
	move._data = nullptr;
	_region = move._region;
	_borderVoxel = move._borderVoxel;
}

RawVolume::RawVolume(const Voxel *data, const voxel::Region &region) {
	initialise(region);
	const size_t size = RawVolume::size(_region);
	core_memcpy((void *)_data, (const void *)data, size);
}

RawVolume::RawVolume(Voxel *data, const voxel::Region &region) : _region(region), _data(data) {
	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");
}

RawVolume::~RawVolume() {
	core_free(_data);
	_data = nullptr;
}

bool RawVolume::move(const glm::ivec3 &shift) {
	const int w = width();
	const int h = height();
	const int d = depth();

	glm::ivec3 t = shift;
	t.x = (t.x % w + w) % w;
	t.y = (t.y % h + h) % h;
	t.z = (t.z % d + d) % d;

	const int hwstride = h * w;
	for (int z = 0; z < d; ++z) {
		const int zhwstride = z * hwstride;
		for (int y = 0; y < h; ++y) {
			const int begin = zhwstride + y * w;
			core::rotate(_data + begin, _data + begin + t.x, _data + zhwstride + (y + 1) * w);
		}
	}

	const int yoffset = t.y * w;
	for (int z = 0; z < d; ++z) {
		core::rotate(_data + z * hwstride, _data + z * hwstride + yoffset, _data + (z + 1) * hwstride);
	}

	core::rotate(_data, _data + t.z * hwstride, _data + d * hwstride);

	return true;
}

Voxel *RawVolume::copyVoxels() const {
	const size_t size = RawVolume::size(_region);
	Voxel *rawCopy = (Voxel *)core_malloc(size);
	core_memcpy((void *)rawCopy, (void *)_data, size);
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
const Voxel &RawVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	if (_region.containsPoint(x, y, z)) {
		const int32_t iLocalXPos = x - _region.getLowerX();
		const int32_t iLocalYPos = y - _region.getLowerY();
		const int32_t iLocalZPos = z - _region.getLowerZ();

		return _data[iLocalXPos + iLocalYPos * width() + iLocalZPos * width() * height()];
	}
	return _borderVoxel;
}

/**
 * @param[in] voxel The value to use for voxels outside the volume.
 */
void RawVolume::setBorderValue(const Voxel &voxel) {
	_borderVoxel = voxel;
}

/**
 * @param x the @c x position of the voxel
 * @param y the @c y position of the voxel
 * @param z the @c z position of the voxel
 * @param voxel the value to which the voxel will be set
 * @return @c true if the voxel was placed, @c false if it was already the same voxel
 */
bool RawVolume::setVoxel(int32_t x, int32_t y, int32_t z, const Voxel &voxel) {
	return setVoxel(glm::ivec3(x, y, z), voxel);
}

/**
 * @param pos the 3D position of the voxel
 * @param voxel the value to which the voxel will be set
 * @return @c true if the voxel was placed, @c false if it was already the same voxel
 */
bool RawVolume::setVoxel(const glm::ivec3 &pos, const Voxel &voxel) {
	const bool inside = _region.containsPoint(pos);
	core_assert_msg(inside, "Position is outside valid region %i:%i:%i (mins[%i:%i:%i], maxs[%i:%i:%i])", pos.x, pos.y,
					pos.z, _region.getLowerX(), _region.getLowerY(), _region.getLowerZ(), _region.getUpperX(),
					_region.getUpperY(), _region.getUpperZ());
	if (!inside) {
		return false;
	}
	const glm::ivec3 &lowerCorner = _region.getLowerCorner();
	const glm::ivec3 localPos = pos - lowerCorner;
	const int index = localPos.x + localPos.y * width() + localPos.z * width() * height();
	if (_data[index].isSame(voxel)) {
		return false;
	}
	_data[index] = voxel;
	return true;
}

void RawVolume::setVoxelUnsafe(const glm::ivec3 &pos, const Voxel &voxel) {
	const glm::ivec3 &lowerCorner = _region.getLowerCorner();
	const glm::ivec3 localPos = pos - lowerCorner;
	const int index = localPos.x + localPos.y * width() + localPos.z * width() * height();
	_data[index] = voxel;
}

/**
 * This function should probably be made internal...
 */
void RawVolume::initialise(const Region &regValidRegion) {
	_region = regValidRegion;

	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");

	// Create the data
	const size_t size = RawVolume::size(_region);
	_data = (Voxel *)core_malloc(size);
	core_assert_msg_always(_data != nullptr, "Failed to allocate the memory for a volume with the dimensions %i:%i:%i",
						   width(), height(), depth());

	// Clear to zeros
	clear();
}

void RawVolume::clear() {
	const size_t size = RawVolume::size(_region);
	core_memset(_data, 0, size);
}

void RawVolume::fill(const voxel::Voxel &voxel) {
	const size_t size = width() * height() * depth();
	for (size_t i = 0; i < size; ++i) {
		_data[i] = voxel;
	}
}

} // namespace voxel
