/**
 * @file
 */

#include "RawVolume.h"
#include "app/Async.h"
#include "core/Algorithm.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include <glm/common.hpp>
#include <limits>

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
		const size_t size = RawVolume::size(_region);
		_data = (Voxel *)core_malloc(size);
		core_memset((void *)_data, 0, size);
	} else if (src.region() == _region) {
		const size_t size = RawVolume::size(_region);
		_data = (Voxel *)core_malloc(size);
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
		const size_t size = RawVolume::size(_region);
		_data = (Voxel *)core_malloc(size);
		const glm::ivec3 &tgtMins = _region.getLowerCorner();
		const glm::ivec3 &tgtMaxs = _region.getUpperCorner();
		const glm::ivec3 &srcMins = src._region.getLowerCorner();

		const int tgtWidth = _region.getWidthInVoxels();
		const int tgtHeight = _region.getHeightInVoxels();
		const int tgtYStride = tgtWidth;
		const int tgtZStride = tgtWidth * tgtHeight;

		const int srcWidth = src._region.getWidthInVoxels();
		const int srcHeight = src._region.getHeightInVoxels();
		const int srcYStride = srcWidth;
		const int srcZStride = srcWidth * srcHeight;

		const int lineLength = tgtMaxs.x - tgtMins.x + 1;
		const size_t lineSize = sizeof(voxel::Voxel) * lineLength;

		for (int z = tgtMins.z; z <= tgtMaxs.z; ++z) {
			const int32_t tgtZPos = z - tgtMins.z;
			const int32_t srcZPos = z - srcMins.z;

			for (int y = tgtMins.y; y <= tgtMaxs.y; ++y) {
				const int32_t tgtYPos = y - tgtMins.y;
				const int32_t srcYPos = y - srcMins.y;

				const int tgtBaseIndex = tgtZPos * tgtZStride + tgtYPos * tgtYStride + (tgtMins.x - _region.getLowerX());
				const int srcBaseIndex = srcZPos * srcZStride + srcYPos * srcYStride + (tgtMins.x - srcMins.x);

				voxel::Voxel* tgtLine = &_data[tgtBaseIndex];
				const voxel::Voxel* srcLine = &src._data[srcBaseIndex];

				core_memcpy((void*)tgtLine, (void*)srcLine, lineSize);

				// Optional air check
				if (onlyAir) {
					if (core::memchr_not(tgtLine, 0, lineSize) != nullptr) {
						*onlyAir = false;
						onlyAir = nullptr; // Disable further checking
					}
				}
			}
		}
	}
}

bool RawVolume::hasFlags(const Region &region, uint8_t flags) const {
	if (!intersects(_region, region)) {
		return false;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	// Flags are at bits 2-3 in the first byte of the 4-byte Voxel struct
	// Create a 32-bit mask for one voxel with the flags bits set
	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	// Create a 64-bit mask for two voxels at once
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;

	const glm::ivec3 &mins = r.getLowerCorner();
	const glm::ivec3 &maxs = r.getUpperCorner();
	const int width = _region.getWidthInVoxels();
	const int height = _region.getHeightInVoxels();
	const int yStride = width;
	const int zStride = width * height;

	const int xStart = mins.x - _region.getLowerX();
	const int lineLength = maxs.x - mins.x + 1;

	for (int z = mins.z; z <= maxs.z; ++z) {
		const int zPos = z - _region.getLowerZ();
		const int zBase = zPos * zStride + xStart;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int yPos = y - _region.getLowerY();
			const int baseIndex = zBase + (yPos * yStride);

			// Process two voxels at a time using 64-bit operations
			const uint64_t *data64 = (const uint64_t *)&_data[baseIndex];
			int i = 0;
			const int pairs = lineLength / 2;
			for (; i < pairs; ++i) {
				if (data64[i] & flagsMask64) {
					return true;
				}
			}
			// Handle remaining voxel if line length is odd
			if (lineLength & 1) {
				const uint32_t *data32 = (const uint32_t *)&_data[baseIndex + pairs * 2];
				if (*data32 & flagsMask32) {
					return true;
				}
			}
		}
	}
	return false;
}

void RawVolume::removeFlags(const Region &region, uint8_t flags) {
	if (!intersects(_region, region)) {
		return;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	// Flags are at bits 2-3 in the first byte of the 4-byte Voxel struct
	// Create a 32-bit mask for one voxel with the flags bits set
	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	// Create a 64-bit mask for two voxels at once
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;
	// Invert the mask for clearing flags
	const uint64_t clearMask64 = ~flagsMask64;
	const uint32_t clearMask32 = ~flagsMask32;

	const glm::ivec3 &mins = r.getLowerCorner();
	const glm::ivec3 &maxs = r.getUpperCorner();
	const int width = _region.getWidthInVoxels();
	const int height = _region.getHeightInVoxels();
	const int yStride = width;
	const int zStride = width * height;

	const int xStart = mins.x - _region.getLowerX();
	const int lineLength = maxs.x - mins.x + 1;

	for (int z = mins.z; z <= maxs.z; ++z) {
		const int zPos = z - _region.getLowerZ();
		const int zBase = zPos * zStride + xStart;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int yPos = y - _region.getLowerY();
			const int baseIndex = zBase + (yPos * yStride);

			// Process two voxels at a time using 64-bit operations
			uint64_t *data64 = (uint64_t *)&_data[baseIndex];
			int i = 0;
			const int pairs = lineLength / 2;
			for (; i < pairs; ++i) {
				data64[i] &= clearMask64;
			}
			// Handle remaining voxel if line length is odd
			if (lineLength & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + pairs * 2];
				*data32 &= clearMask32;
			}
		}
	}
}

void RawVolume::setFlags(const Region &region, uint8_t flags) {
	if (!intersects(_region, region)) {
		return;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	// Flags are at bits 2-3 in the first byte of the 4-byte Voxel struct
	// Create a 32-bit mask for one voxel with the flags bits set
	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	// Create a 64-bit mask for two voxels at once
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;

	const glm::ivec3 &mins = r.getLowerCorner();
	const glm::ivec3 &maxs = r.getUpperCorner();
	const int width = _region.getWidthInVoxels();
	const int height = _region.getHeightInVoxels();
	const int yStride = width;
	const int zStride = width * height;

	const int xStart = mins.x - _region.getLowerX();
	const int lineLength = maxs.x - mins.x + 1;

	for (int z = mins.z; z <= maxs.z; ++z) {
		const int zPos = z - _region.getLowerZ();
		const int zBase = zPos * zStride + xStart;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int yPos = y - _region.getLowerY();
			const int baseIndex = zBase + (yPos * yStride);

			// Process two voxels at a time using 64-bit operations
			uint64_t *data64 = (uint64_t *)&_data[baseIndex];
			int i = 0;
			const int pairs = lineLength / 2;
			for (; i < pairs; ++i) {
				data64[i] |= flagsMask64;
			}
			// Handle remaining voxel if line length is odd
			if (lineLength & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + pairs * 2];
				*data32 |= flagsMask32;
			}
		}
	}
}

bool RawVolume::isEmpty(const Region &region) const {
	core_trace_scoped(RawVolumeIsEmpty);
	if (!intersects(_region, region)) {
		return true;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	const glm::ivec3 &mins = r.getLowerCorner();
	const glm::ivec3 &maxs = r.getUpperCorner();
	const int width = _region.getWidthInVoxels();
	const int height = _region.getHeightInVoxels();
	const int yStride = width;
	const int zStride = width * height;

	const int lineLength = maxs.x - mins.x + 1;
	const int voxelLineSize = sizeof(voxel::Voxel) * lineLength;
	const int xStart = (mins.x - _region.getLowerX());
	for (int z = mins.z; z <= maxs.z; ++z) {
		const int zPos = z - _region.getLowerZ();
		const int zBase = zPos * zStride + xStart;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int yPos = y - _region.getLowerY();
			const int baseIndex = zBase + (yPos * yStride);
			const voxel::Voxel* dataLine = &_data[baseIndex];

			if (core::memchr_not(dataLine, 0, voxelLineSize) != nullptr) {
				return false;
			}
		}
	}

	return true;
}

bool RawVolume::copyInto(const RawVolume &src) {
	if (_region == src.region()) {
		core_memcpy((void *)_data, (void *)src._data, RawVolume::size(_region));
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
	if (!src.region().containsRegion(srcRegion)) {
		srcRegion.cropTo(src.region());
	}
	if (!_region.containsRegion(srcRegion)) {
		srcRegion.cropTo(_region);
	}

	app::for_parallel(srcRegion.getLowerZ(), srcRegion.getUpperZ() + 1, [&src, this, srcRegion](int start, int end) {
		const glm::ivec3 &mins = srcRegion.getLowerCorner();
		const glm::ivec3 &maxs = srcRegion.getUpperCorner();
		const voxel::Region &fullSrcRegion = src.region();
		const int width = _region.getWidthInVoxels();
		const int height = _region.getHeightInVoxels();

		const int tgtYStride = width;
		const int tgtZStride = width * height;

		const int srcWidth = fullSrcRegion.getWidthInVoxels();
		const int srcHeight = fullSrcRegion.getHeightInVoxels();
		const int srcXOffset = mins.x - fullSrcRegion.getLowerX();
		const int srcYOffset = mins.y - fullSrcRegion.getLowerY();
		const int srcZOffset = mins.z - fullSrcRegion.getLowerZ();
		const int srcYStride = srcWidth;
		const int srcZStride = srcWidth * srcHeight;
		const int tgtXOffset = mins.x - _region.getLowerX();

		const int lineLength = maxs.x - mins.x + 1;
		const size_t lineSize = sizeof(voxel::Voxel) * lineLength;
		for (int z = start; z < end; ++z) {
			const int tgtZPos = z - _region.getLowerZ();
			const int srcZPos = srcZOffset + z - mins.z;
			const int srcXZBaseIndex = srcXOffset + srcZPos * srcZStride;
			const int tgtXZBaseIndex = tgtZPos * tgtZStride + tgtXOffset;

			for (int y = mins.y; y <= maxs.y; ++y) {
				const int tgtYPos = y - _region.getLowerY();
				const int srcYPos = srcYOffset + y - mins.y;

				const int tgtBaseIndex = tgtXZBaseIndex + tgtYPos * tgtYStride;
				const int srcBaseIndex = srcXZBaseIndex + srcYPos * srcYStride;

				const voxel::Voxel* srcLine = &src._data[srcBaseIndex];
				voxel::Voxel* tgtLine = &_data[tgtBaseIndex];

				core_memcpy((void*)tgtLine, (void*)srcLine, lineSize);
			}
		}
	});
	return true;
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

		return _data[iLocalXPos + iLocalYPos * width() + iLocalZPos * _region.stride()];
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

bool RawVolume::setVoxel(int idx, const Voxel &voxel) {
	if (idx < 0 || idx >= _region.stride() * depth()) {
		return false; // Index out of bounds
	}
	if (_data[idx] == voxel) {
		return false;
	}
	_data[idx] = voxel;
	return true;
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
	const int index = localPos.x + localPos.y * width() + localPos.z * _region.stride();
	if (_data[index] == voxel) {
		return false;
	}
	_data[index] = voxel;
	return true;
}

void RawVolume::setVoxelUnsafe(const glm::ivec3 &pos, const Voxel &voxel) {
	const glm::ivec3 &lowerCorner = _region.getLowerCorner();
	const glm::ivec3 localPos = pos - lowerCorner;
	const int index = localPos.x + localPos.y * width() + localPos.z * _region.stride();
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
	Voxel voxel;
	fill(voxel);
}

void RawVolume::fill(const voxel::Voxel &voxel) {
	if (!_region.isValid()) {
		return;
	}
	const size_t size = _region.stride() * depth();
	core_memset4((void *)_data, *(uint32_t*)&voxel, size);
	static_assert(sizeof(Voxel) == 4, "Voxel is expected to be 4 bytes");
}

} // namespace voxel
