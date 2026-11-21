/**
 * @file
 */

#include "RawVolume.h"
#include "app/ForParallel.h"
#include "core/Algorithm.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Atomic.h"
#include "private/SparseRawVolumeStorage.h"
#include <SDL3/SDL_stdinc.h>
#include <glm/common.hpp>

namespace voxel {

namespace {

constexpr int DefaultChunkedStorageThreshold = 256;
core::AtomicInt gChunkedStorageThreshold{DefaultChunkedStorageThreshold};

static inline bool shouldUseSparseStorage(const Region &region) {
	const int threshold = (int)gChunkedStorageThreshold;
	const int64_t limit = (int64_t)threshold * (int64_t)threshold * (int64_t)threshold;
	return region.voxels() >= limit;
}

} // namespace

void RawVolume::setChunkedStorageThreshold(int sideLength) {
	core_assert(sideLength > 0);
	if (sideLength > 0) {
		gChunkedStorageThreshold = sideLength;
	}
}

int RawVolume::chunkedStorageThreshold() {
	return (int)gChunkedStorageThreshold;
}

size_t RawVolume::size(const Region &region) {
	if (!region.isValid()) {
		return 0;
	}
	const uint64_t w = (uint64_t)region.getWidthInVoxels();
	const uint64_t h = (uint64_t)region.getHeightInVoxels();
	const uint64_t d = (uint64_t)region.getDepthInVoxels();
	if (w == 0u || h == 0u || d == 0u) {
		return 0;
	}
	if (w > SIZE_MAX / h) {
		return 0;
	}
	const size_t wh = (size_t)(w * h);
	if (d > SIZE_MAX / wh) {
		return 0;
	}
	const size_t count = wh * (size_t)d;
	if (count > SIZE_MAX / sizeof(Voxel)) {
		return 0;
	}
	return count * sizeof(Voxel);
}

RawVolume::RawVolume(const Region &regValid) : _region(regValid) {
	initialise(regValid);
}

RawVolume::RawVolume(const RawVolume *copy) : RawVolume(*copy) {
}

RawVolume::RawVolume(const RawVolume &copy) : _region(copy.region()) {
	setBorderValue(copy.borderValue());
	if (copy._storage != nullptr) {
		_storage = new SparseRawVolumeStorage(*copy._storage);
		return;
	}
	if (copy._data == nullptr) {
		return;
	}
	const size_t bytes = RawVolume::size(_region);
	_data = (Voxel *)core_malloc(bytes);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for volume copy", (uint64_t)bytes);
		return;
	}
	core_memcpy((void *)_data, (const void *)copy._data, bytes);
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

RawVolume::RawVolume(const RawVolume &src, const core::Buffer<Region> &copyRegions) : _region(accumulate(copyRegions)) {
	_region.cropTo(src.region());
	setBorderValue(src.borderValue());
	initialise(_region);

	for (const Region &copyRegion : copyRegions) {
		copyInto(src, copyRegion);
	}
}

RawVolume::RawVolume(const RawVolume &src, const Region &region, bool *onlyAir) : _region(region) {
	core_trace_scoped(RawVolumeCopyRegion);
	core_assert(region.isValid());
	setBorderValue(src.borderValue());
	if (!intersects(src.region(), _region)) {
		if (onlyAir) {
			*onlyAir = true;
		}
		initialise(_region);
		return;
	}
	if (src.region() == _region) {
		if (src._storage != nullptr) {
			_storage = new SparseRawVolumeStorage(*src._storage);
		} else if (src._data != nullptr) {
			const size_t bytes = RawVolume::size(_region);
			_data = (Voxel *)core_malloc(bytes);
			if (_data == nullptr) {
				Log::error("Failed to allocate %" SDL_PRIu64 " bytes for volume copy", (uint64_t)bytes);
				return;
			}
			core_memcpy((void *)_data, (const void *)src._data, bytes);
		}
		if (onlyAir) {
			*onlyAir = false;
		}
		return;
	}
	if (!src.region().containsRegion(_region)) {
		_region.cropTo(src._region);
	}
	if (onlyAir) {
		*onlyAir = true;
	}
	initialise(_region);
	if (!valid()) {
		return;
	}
	copyInto(src, _region);
	if (onlyAir != nullptr) {
		*onlyAir = isEmpty(_region);
	}
}

// Scan a contiguous block of voxels for any matching flags.
static CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool scanFlagsLinear(const Voxel *data, int64_t count,
																	   uint32_t mask) {
	static_assert(sizeof(Voxel) == 4u, "Voxel size must be 4 bytes for scanFlagsLinear");
	const uint32_t *p = (const uint32_t *)data;
	int64_t i = 0;
	for (; i + 7 < count; i += 8) {
		const uint32_t acc = p[i] | p[i + 1] | p[i + 2] | p[i + 3] | p[i + 4] | p[i + 5] | p[i + 6] | p[i + 7];
		if (acc & mask) {
			return true;
		}
	}
	uint32_t acc = 0;
	for (; i < count; i++) {
		acc |= p[i];
	}
	return (acc & mask) != 0;
}

bool RawVolume::hasFlags(const Region &region, uint8_t flags) const {
	if (!intersects(_region, region) || !valid()) {
		return false;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	if (_storage != nullptr) {
		const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
		const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
		return _storage->hasFlags(lMins, lMaxs, flags);
	}

	const uint32_t flagsMask = (uint32_t)(flags & 0x3) << 2;

	const int w = _region.getWidthInVoxels();
	const int h = _region.getHeightInVoxels();
	const int scanWidth = r.getWidthInVoxels();
	const int scanHeight = r.getHeightInVoxels();
	const int scanDepth = r.getDepthInVoxels();

	if (scanWidth == w && scanHeight == h) {
		const int64_t zStart = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
		const Voxel *start = _data + zStart * (int64_t)w * (int64_t)h;
		const int64_t total = (int64_t)scanDepth * w * h;
		return scanFlagsLinear(start, total, flagsMask);
	}

	if (scanWidth == w) {
		const int64_t yStart = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
		const int64_t zStride = (int64_t)w * h;
		const int64_t rowsPerSlice = (int64_t)scanHeight * w;
		const int64_t zOff = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
		for (int z = 0; z < scanDepth; ++z) {
			const Voxel *start = _data + (zOff + z) * zStride + yStart * w;
			if (scanFlagsLinear(start, rowsPerSlice, flagsMask)) {
				return true;
			}
		}
		return false;
	}

	const int64_t yStride = w;
	const int64_t zStride = (int64_t)w * h;
	const int64_t xStart = (int64_t)r.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t yOff = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t zOff = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();

	for (int z = 0; z < scanDepth; ++z) {
		const int64_t zBase = (zOff + z) * zStride + xStart;
		for (int y = 0; y < scanHeight; ++y) {
			const int64_t baseIndex = zBase + (yOff + y) * yStride;
			if (scanFlagsLinear(_data + baseIndex, scanWidth, flagsMask)) {
				return true;
			}
		}
	}
	return false;
}

Region RawVolume::regionForFlag(uint8_t flag) const {
	if (!valid()) {
		return Region::InvalidRegion;
	}
	if (_storage != nullptr) {
		glm::ivec3 lMins;
		glm::ivec3 lMaxs;
		if (!_storage->regionForFlag(flag, lMins, lMaxs)) {
			return Region::InvalidRegion;
		}
		const glm::ivec3 &lower = _region.getLowerCorner();
		return Region((int32_t)((int64_t)lower.x + lMins.x), (int32_t)((int64_t)lower.y + lMins.y),
					  (int32_t)((int64_t)lower.z + lMins.z), (int32_t)((int64_t)lower.x + lMaxs.x),
					  (int32_t)((int64_t)lower.y + lMaxs.y), (int32_t)((int64_t)lower.z + lMaxs.z));
	}

	const int w = _region.getWidthInVoxels();
	const int h = _region.getHeightInVoxels();
	const int d = _region.getDepthInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = (int64_t)w * h;
	const uint32_t flagsMask = (uint32_t)(flag & 0x3) << 2;

	int minZ = -1;
	for (int z = 0; z < d; ++z) {
		if (scanFlagsLinear(_data + z * zStride, zStride, flagsMask)) {
			minZ = z;
			break;
		}
	}
	if (minZ < 0) {
		return Region::InvalidRegion;
	}

	int maxZ = minZ;
	for (int z = d - 1; z > minZ; --z) {
		if (scanFlagsLinear(_data + z * zStride, zStride, flagsMask)) {
			maxZ = z;
			break;
		}
	}

	int minY = h, maxY = -1;
	int minX = w, maxX = -1;

	for (int z = minZ; z <= maxZ; ++z) {
		const int64_t zBase = z * zStride;
		if (z != minZ && z != maxZ && !scanFlagsLinear(_data + zBase, zStride, flagsMask)) {
			continue;
		}
		for (int y = 0; y < minY; ++y) {
			const int64_t base = zBase + y * yStride;
			if (!scanFlagsLinear(_data + base, w, flagsMask)) {
				continue;
			}
			minY = y;
			for (int x = 0; x < minX; ++x) {
				if (_data[base + x].getFlags() & flag) {
					minX = x;
					break;
				}
			}
			for (int x = w - 1; x > maxX; --x) {
				if (_data[base + x].getFlags() & flag) {
					maxX = x;
					break;
				}
			}
			break;
		}
		for (int y = h - 1; y > maxY; --y) {
			const int64_t base = zBase + y * yStride;
			if (!scanFlagsLinear(_data + base, w, flagsMask)) {
				continue;
			}
			maxY = y;
			for (int x = 0; x < minX; ++x) {
				if (_data[base + x].getFlags() & flag) {
					minX = x;
					break;
				}
			}
			for (int x = w - 1; x > maxX; --x) {
				if (_data[base + x].getFlags() & flag) {
					maxX = x;
					break;
				}
			}
			break;
		}
		if (minX > 0 || maxX < w - 1) {
			for (int y = minY; y <= maxY; ++y) {
				const int64_t base = zBase + y * yStride;
				if (!scanFlagsLinear(_data + base, w, flagsMask)) {
					continue;
				}
				for (int x = 0; x < minX; ++x) {
					if (_data[base + x].getFlags() & flag) {
						minX = x;
						break;
					}
				}
				for (int x = w - 1; x > maxX; --x) {
					if (_data[base + x].getFlags() & flag) {
						maxX = x;
						break;
					}
				}
			}
		}
	}

	const glm::ivec3 &lower = _region.getLowerCorner();
	return Region((int32_t)((int64_t)lower.x + minX), (int32_t)((int64_t)lower.y + minY),
				  (int32_t)((int64_t)lower.z + minZ), (int32_t)((int64_t)lower.x + maxX),
				  (int32_t)((int64_t)lower.y + maxY), (int32_t)((int64_t)lower.z + maxZ));
}

void RawVolume::removeFlags(const Region &region, uint8_t flags) {
	if (!intersects(_region, region) || !valid()) {
		return;
	}
	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}
	if (_storage != nullptr) {
		const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
		const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
		_storage->modifyFlags(lMins, lMaxs, flags, 1);
		return;
	}

	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;
	const uint64_t clearMask64 = ~flagsMask64;
	const uint32_t clearMask32 = ~flagsMask32;

	const int64_t w = _region.getWidthInVoxels();
	const int64_t h = _region.getHeightInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = w * h;
	const int64_t xStart = (int64_t)r.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t yStart = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t zStart = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
	const int lineLength = r.getWidthInVoxels();
	const int yCount = r.getHeightInVoxels();
	const int zCount = r.getDepthInVoxels();

	for (int z = 0; z < zCount; ++z) {
		const int64_t zBase = (zStart + z) * zStride + xStart;
		for (int y = 0; y < yCount; ++y) {
			const int64_t baseIndex = zBase + (yStart + y) * yStride;
			int offset = 0;
			int remaining = lineLength;
			if ((baseIndex & 1) && remaining > 0) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex];
				*data32 &= clearMask32;
				offset = 1;
				remaining--;
			}
			const int pairs = remaining / 2;
			uint64_t *data64 = (uint64_t *)&_data[baseIndex + offset];
			for (int i = 0; i < pairs; ++i) {
				data64[i] &= clearMask64;
			}
			if (remaining & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + offset + pairs * 2];
				*data32 &= clearMask32;
			}
		}
	}
}

void RawVolume::toggleFlags(const Region &region, uint8_t flags) {
	if (!intersects(_region, region) || !valid()) {
		return;
	}
	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}
	if (_storage != nullptr) {
		const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
		const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
		_storage->modifyFlags(lMins, lMaxs, flags, 2);
		return;
	}

	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;

	const int64_t w = _region.getWidthInVoxels();
	const int64_t h = _region.getHeightInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = w * h;
	const int64_t xStart = (int64_t)r.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t yStart = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t zStart = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
	const int lineLength = r.getWidthInVoxels();
	const int yCount = r.getHeightInVoxels();
	const int zCount = r.getDepthInVoxels();

	for (int z = 0; z < zCount; ++z) {
		const int64_t zBase = (zStart + z) * zStride + xStart;
		for (int y = 0; y < yCount; ++y) {
			const int64_t baseIndex = zBase + (yStart + y) * yStride;
			int offset = 0;
			int remaining = lineLength;
			if ((baseIndex & 1) && remaining > 0) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex];
				*data32 ^= flagsMask32;
				offset = 1;
				remaining--;
			}
			const int pairs = remaining / 2;
			uint64_t *data64 = (uint64_t *)&_data[baseIndex + offset];
			for (int i = 0; i < pairs; ++i) {
				data64[i] ^= flagsMask64;
			}
			if (remaining & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + offset + pairs * 2];
				*data32 ^= flagsMask32;
			}
		}
	}
}

void RawVolume::setFlags(const Region &region, uint8_t flags) {
	if (!intersects(_region, region) || !valid()) {
		return;
	}
	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}
	if (_storage != nullptr) {
		const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
		const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
		_storage->modifyFlags(lMins, lMaxs, flags, 0);
		return;
	}

	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;

	const int64_t w = _region.getWidthInVoxels();
	const int64_t h = _region.getHeightInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = w * h;
	const int64_t xStart = (int64_t)r.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t yStart = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t zStart = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
	const int lineLength = r.getWidthInVoxels();
	const int yCount = r.getHeightInVoxels();
	const int zCount = r.getDepthInVoxels();

	for (int z = 0; z < zCount; ++z) {
		const int64_t zBase = (zStart + z) * zStride + xStart;
		for (int y = 0; y < yCount; ++y) {
			const int64_t baseIndex = zBase + (yStart + y) * yStride;
			int offset = 0;
			int remaining = lineLength;
			if ((baseIndex & 1) && remaining > 0) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex];
				*data32 |= flagsMask32;
				offset = 1;
				remaining--;
			}
			const int pairs = remaining / 2;
			uint64_t *data64 = (uint64_t *)&_data[baseIndex + offset];
			for (int i = 0; i < pairs; ++i) {
				data64[i] |= flagsMask64;
			}
			if (remaining & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + offset + pairs * 2];
				*data32 |= flagsMask32;
			}
		}
	}
}

bool RawVolume::isEmpty(const Region &region) const {
	core_trace_scoped(RawVolumeIsEmpty);
	if (!intersects(_region, region) || !valid()) {
		return true;
	}

	voxel::Region r = region;
	if (!_region.containsRegion(r)) {
		r.cropTo(_region);
	}

	if (_storage != nullptr) {
		const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
		const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
		return _storage->isEmpty(lMins, lMaxs);
	}

	const int64_t w = _region.getWidthInVoxels();
	const int64_t h = _region.getHeightInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = w * h;

	const int lineLength = r.getWidthInVoxels();
	const size_t voxelLineSize = sizeof(voxel::Voxel) * (size_t)lineLength;
	const int64_t xStart = (int64_t)r.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t yStart = (int64_t)r.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t zStart = (int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ();
	const int yCount = r.getHeightInVoxels();
	const int zCount = r.getDepthInVoxels();

	for (int z = 0; z < zCount; ++z) {
		const int64_t zBase = (zStart + z) * zStride + xStart;
		for (int y = 0; y < yCount; ++y) {
			const int64_t baseIndex = zBase + (yStart + y) * yStride;
			if (core::memchr_not(&_data[baseIndex], 0, voxelLineSize) != nullptr) {
				return false;
			}
		}
	}

	return true;
}

bool RawVolume::equals(const RawVolume &other) const {
	if (_region != other._region) {
		return false;
	}
	if (!valid() || !other.valid()) {
		return valid() == other.valid();
	}
	if (_data != nullptr && other._data != nullptr) {
		return core_memcmp(_data, other._data, RawVolume::size(_region)) == 0;
	}
	if (_storage != nullptr && other._storage != nullptr) {
		return _storage->equals(*other._storage);
	}
	if (_storage != nullptr) {
		return _storage->equalsDense(other._data);
	}
	return other._storage->equalsDense(_data);
}

bool RawVolume::computeSolidBounds(glm::ivec3 &mins, glm::ivec3 &maxs) const {
	if (!valid()) {
		return false;
	}
	if (_storage != nullptr) {
		glm::ivec3 lMins;
		glm::ivec3 lMaxs;
		if (!_storage->computeSolidBounds(lMins, lMaxs)) {
			return false;
		}
		const glm::ivec3 &lower = _region.getLowerCorner();
		mins = glm::ivec3((int32_t)((int64_t)lower.x + lMins.x), (int32_t)((int64_t)lower.y + lMins.y),
						  (int32_t)((int64_t)lower.z + lMins.z));
		maxs = glm::ivec3((int32_t)((int64_t)lower.x + lMaxs.x), (int32_t)((int64_t)lower.y + lMaxs.y),
						  (int32_t)((int64_t)lower.z + lMaxs.z));
		return true;
	}

	const int w = _region.getWidthInVoxels();
	const int h = _region.getHeightInVoxels();
	const int d = _region.getDepthInVoxels();
	const int64_t yStride = w;
	const int64_t zStride = (int64_t)w * h;
	const size_t lineSize = sizeof(voxel::Voxel) * (size_t)w;

	int minZ = d, maxZ = -1;
	int minY = h, maxY = -1;
	int minX = w, maxX = -1;

	for (int z = 0; z < d; ++z) {
		const int64_t zBase = (int64_t)z * zStride;
		for (int y = 0; y < h; ++y) {
			const Voxel *lineStart = &_data[zBase + (int64_t)y * yStride];
			const void *found = core::memchr_not(lineStart, 0, lineSize);
			if (found == nullptr) {
				continue;
			}
			int firstX = (int)(((const uint8_t *)found - (const uint8_t *)lineStart) / sizeof(Voxel));
			while (firstX < w && isAir(lineStart[firstX].getMaterial())) {
				++firstX;
			}
			if (firstX == w) {
				continue;
			}
			minZ = core_min(minZ, z);
			maxZ = core_max(maxZ, z);
			minY = core_min(minY, y);
			maxY = core_max(maxY, y);
			minX = core_min(minX, firstX);
			for (int x = w - 1; x > maxX; --x) {
				if (!isAir(lineStart[x].getMaterial())) {
					maxX = x;
					break;
				}
			}
		}
	}
	if (maxZ < 0) {
		return false;
	}
	const glm::ivec3 &lower = _region.getLowerCorner();
	mins = glm::ivec3((int32_t)((int64_t)lower.x + minX), (int32_t)((int64_t)lower.y + minY),
					  (int32_t)((int64_t)lower.z + minZ));
	maxs = glm::ivec3((int32_t)((int64_t)lower.x + maxX), (int32_t)((int64_t)lower.y + maxY),
					  (int32_t)((int64_t)lower.z + maxZ));
	return true;
}

void RawVolume::readRegion(const Region &region, Voxel *target) const {
	if (target == nullptr || !region.isValid() || !valid()) {
		return;
	}
	const glm::ivec3 sz = region.getDimensionsInVoxels();
	if (_region.containsRegion(region)) {
		const glm::ivec3 lMins((int)((int64_t)region.getLowerX() - (int64_t)_region.getLowerX()),
							   (int)((int64_t)region.getLowerY() - (int64_t)_region.getLowerY()),
							   (int)((int64_t)region.getLowerZ() - (int64_t)_region.getLowerZ()));
		if (_storage != nullptr) {
			_storage->readRegion(lMins, sz, target);
			return;
		}
		const int64_t srcW = _region.getWidthInVoxels();
		const int64_t srcStride = _region.stride();
		const int64_t dstStride = (int64_t)sz.x * sz.y;
		const size_t rowBytes = (size_t)sz.x * sizeof(Voxel);
		for (int z = 0; z < sz.z; ++z) {
			const int64_t srcZ = ((int64_t)lMins.z + z) * srcStride + lMins.x;
			const int64_t dstZ = (int64_t)z * dstStride;
			for (int y = 0; y < sz.y; ++y) {
				core_memcpy((void *)(target + dstZ + (int64_t)y * sz.x),
							(const void *)(_data + srcZ + ((int64_t)lMins.y + y) * srcW), rowBytes);
			}
		}
		return;
	}

	// Partial or non-overlapping region: prefill with borderValue then copy intersection
	const size_t total = (size_t)sz.x * (size_t)sz.y * (size_t)sz.z;
	core_memset4((void *)target, SparseRawVolumeStorage::voxelBits(_borderVoxel), total);
	Region overlap = region;
	if (!overlap.cropTo(_region)) {
		return;
	}
	const glm::ivec3 oSz = overlap.getDimensionsInVoxels();
	const int64_t offX = (int64_t)overlap.getLowerX() - (int64_t)region.getLowerX();
	const int64_t offY = (int64_t)overlap.getLowerY() - (int64_t)region.getLowerY();
	const int64_t offZ = (int64_t)overlap.getLowerZ() - (int64_t)region.getLowerZ();
	const int64_t dstStride = (int64_t)sz.x * sz.y;
	for (int z = 0; z < oSz.z; ++z) {
		for (int y = 0; y < oSz.y; ++y) {
			Voxel *dst = target + (offZ + z) * dstStride + (offY + y) * sz.x + offX;
			for (int x = 0; x < oSz.x; ++x) {
				dst[x].overwrite(voxel((int32_t)((int64_t)overlap.getLowerX() + x),
									   (int32_t)((int64_t)overlap.getLowerY() + y),
									   (int32_t)((int64_t)overlap.getLowerZ() + z)));
			}
		}
	}
}

bool RawVolume::copyInto(const RawVolume &src) {
	if (!valid() || !src.valid()) {
		return false;
	}
	if (_region == src.region()) {
		if (_data != nullptr && src._data != nullptr) {
			core_memcpy((void *)_data, (const void *)src._data, RawVolume::size(_region));
			return true;
		}
		if (_storage != nullptr && src._storage != nullptr) {
			delete _storage;
			_storage = new SparseRawVolumeStorage(*src._storage);
			return _storage->valid();
		}
		if (_data != nullptr && src._storage != nullptr) {
			src._storage->copyTo(_data);
			return true;
		}
	}
	voxel::Region srcRegion = src.region();
	return copyInto(src, srcRegion);
}

bool RawVolume::copyInto(const RawVolume &src, const voxel::Region &region) {
	core_trace_scoped(RawVolumeCopyInto);
	if (!valid() || !src.valid() || !intersects(_region, region)) {
		return false;
	}
	voxel::Region srcRegion = region;
	if (!srcRegion.cropTo(src.region())) {
		return false;
	}
	if (!srcRegion.cropTo(_region)) {
		return false;
	}

	const int lineLength = srcRegion.getWidthInVoxels();
	const int yCount = srcRegion.getHeightInVoxels();
	const int zCount = srcRegion.getDepthInVoxels();
	const int64_t tgtXStart = (int64_t)srcRegion.getLowerX() - (int64_t)_region.getLowerX();
	const int64_t tgtYStart = (int64_t)srcRegion.getLowerY() - (int64_t)_region.getLowerY();
	const int64_t tgtZStart = (int64_t)srcRegion.getLowerZ() - (int64_t)_region.getLowerZ();
	const int64_t srcXStart = (int64_t)srcRegion.getLowerX() - (int64_t)src._region.getLowerX();
	const int64_t srcYStart = (int64_t)srcRegion.getLowerY() - (int64_t)src._region.getLowerY();
	const int64_t srcZStart = (int64_t)srcRegion.getLowerZ() - (int64_t)src._region.getLowerZ();

	// Fast dense -> dense path
	if (_data != nullptr && src._data != nullptr) {
		app::for_parallel(0, zCount, [&](int start, int end) {
			const int64_t tgtW = _region.getWidthInVoxels();
			const int64_t tgtZStride = _region.stride();
			const int64_t srcW = src._region.getWidthInVoxels();
			const int64_t srcZStride = src._region.stride();
			const size_t lineSize = sizeof(voxel::Voxel) * (size_t)lineLength;
			for (int z = start; z < end; ++z) {
				const int64_t srcXZBase = srcXStart + (srcZStart + z) * srcZStride;
				const int64_t tgtXZBase = tgtXStart + (tgtZStart + z) * tgtZStride;
				for (int y = 0; y < yCount; ++y) {
					core_memcpy((void *)&_data[tgtXZBase + (tgtYStart + y) * tgtW],
								(const void *)&src._data[srcXZBase + (srcYStart + y) * srcW], lineSize);
				}
			}
		});
		return true;
	}

	// Fast sparse -> dense path: read slices directly from SparseRawVolumeStorage
	if (_data != nullptr && src._storage != nullptr) {
		const int64_t tgtW = _region.getWidthInVoxels();
		const int64_t tgtZStride = _region.stride();
		app::for_parallel(0, zCount, [&](int start, int end) {
			core::DynamicArray<Voxel> rowBuf;
			rowBuf.resize((size_t)lineLength);
			for (int z = start; z < end; ++z) {
				const int64_t tgtXZBase = tgtXStart + (tgtZStart + z) * tgtZStride;
				for (int y = 0; y < yCount; ++y) {
					src._storage->readRegion(glm::ivec3((int)srcXStart, (int)(srcYStart + y), (int)(srcZStart + z)),
											 glm::ivec3(lineLength, 1, 1), rowBuf.data());
					core_memcpy((void *)&_data[tgtXZBase + (tgtYStart + y) * tgtW], (const void *)rowBuf.data(),
								(size_t)lineLength * sizeof(Voxel));
				}
			}
		});
		return true;
	}

	// Target is sparse: parallel 8x8x8 stack-tile copy + lock-free prepareBrick + pointer-transfer merge
	const int bxMin = (int)tgtXStart >> SparseChunk::BrickShift;
	const int byMin = (int)tgtYStart >> SparseChunk::BrickShift;
	const int bzMin = (int)tgtZStart >> SparseChunk::BrickShift;
	const int bxMax = (int)(tgtXStart + lineLength - 1) >> SparseChunk::BrickShift;
	const int byMax = (int)(tgtYStart + yCount - 1) >> SparseChunk::BrickShift;
	const int bzMax = (int)(tgtZStart + zCount - 1) >> SparseChunk::BrickShift;
	const uint32_t defaultBits = SparseRawVolumeStorage::voxelBits(_storage->defaultVoxel());
	const bool sameAirDefault =
		src.isSparseAirDefault() && SparseRawVolumeStorage::voxelBits(src._storage->defaultVoxel()) == defaultBits;

	core_trace_mutex(core::Lock, mergeLock, "RawVolumeCopyMerge");
	core::DynamicArray<SparseRawVolumeStorage::PreparedChunkBrick> allPrepared;

	app::for_parallel(bzMin, bzMax + 1, [&](int bzStart, int bzEnd) {
		core::DynamicArray<SparseRawVolumeStorage::PreparedChunkBrick> localPrepared;
		Voxel tile[SparseChunk::BrickVoxels];
		uint64_t mask[SparseChunk::BrickMaskWords];
		Voxel srcTile[SparseChunk::BrickVoxels];
		for (int bz = bzStart; bz < bzEnd; ++bz) {
			const int localZ = bz << SparseChunk::BrickShift;
			const int cz0 = core_max(localZ, (int)tgtZStart);
			const int cz1 = core_min(localZ + SparseChunk::BrickSide - 1, (int)(tgtZStart + zCount - 1));
			for (int by = byMin; by <= byMax; ++by) {
				const int localY = by << SparseChunk::BrickShift;
				const int cy0 = core_max(localY, (int)tgtYStart);
				const int cy1 = core_min(localY + SparseChunk::BrickSide - 1, (int)(tgtYStart + yCount - 1));
				for (int bx = bxMin; bx <= bxMax; ++bx) {
					const int localX = bx << SparseChunk::BrickShift;
					const int cx0 = core_max(localX, (int)tgtXStart);
					const int cx1 = core_min(localX + SparseChunk::BrickSide - 1, (int)(tgtXStart + lineLength - 1));

					const int sx0 = (int)(srcXStart + (cx0 - tgtXStart));
					const int sy0 = (int)(srcYStart + (cy0 - tgtYStart));
					const int sz0 = (int)(srcZStart + (cz0 - tgtZStart));
					const int sxLen = cx1 - cx0 + 1;
					const int syLen = cy1 - cy0 + 1;
					const int szLen = cz1 - cz0 + 1;

					// An absent source and destination brick already hold the same value.
					if (sameAirDefault &&
						!src._storage->hasOccupiedVoxelsInBox(glm::ivec3(sx0, sy0, sz0),
															  glm::ivec3(sx0 + sxLen - 1, sy0 + syLen - 1, sz0 + szLen - 1)) &&
						!_storage->hasOccupiedVoxelsInBox(glm::ivec3(localX, localY, localZ),
														  glm::ivec3(localX + SparseChunk::BrickSide - 1,
																	 localY + SparseChunk::BrickSide - 1,
																	 localZ + SparseChunk::BrickSide - 1))) {
						continue;
					}

					int count = 0;
					const bool fullBrick =
						(sxLen == SparseChunk::BrickSide && syLen == SparseChunk::BrickSide && szLen == SparseChunk::BrickSide);
					if (!fullBrick) {
						_storage->readBrickTile(localX, localY, localZ, tile, mask, count);
					} else {
						core_memset((void *)tile, 0, sizeof(tile));
						for (int k = 0; k < SparseChunk::BrickMaskWords; ++k) {
							mask[k] = 0u;
						}
					}

					if (src._data != nullptr) {
						const int64_t srcW = src._region.getWidthInVoxels();
						const int64_t srcZStride = src._region.stride();
						for (int dz = 0; dz < szLen; ++dz) {
							const int lz = (cz0 - localZ) + dz;
							const int64_t srcZBase = (int64_t)(sz0 + dz) * srcZStride + sx0;
							for (int dy = 0; dy < syLen; ++dy) {
								const int ly = (cy0 - localY) + dy;
								const Voxel *srcRow = &src._data[srcZBase + (int64_t)(sy0 + dy) * srcW];
								for (int dx = 0; dx < sxLen; ++dx) {
									const int lx = (cx0 - localX) + dx;
									const int idx = (lz << 6) | (ly << 3) | lx;
									const uint64_t bit = (uint64_t)1u << ((ly << 3) | lx);
									const bool wasSet = (mask[lz] & bit) != 0u;
									const bool nowSet = (SparseRawVolumeStorage::voxelBits(srcRow[dx]) != defaultBits);
									tile[idx].overwrite(srcRow[dx]);
									if (nowSet && !wasSet) {
										mask[lz] |= bit;
										++count;
									} else if (!nowSet && wasSet) {
										mask[lz] &= ~bit;
										--count;
									}
								}
							}
						}
					} else {
						src._storage->readRegion(glm::ivec3(sx0, sy0, sz0), glm::ivec3(sxLen, syLen, szLen), srcTile);
						for (int dz = 0; dz < szLen; ++dz) {
							const int lz = (cz0 - localZ) + dz;
							for (int dy = 0; dy < syLen; ++dy) {
								const int ly = (cy0 - localY) + dy;
								const Voxel *srcRow = srcTile + (dz * syLen + dy) * sxLen;
								for (int dx = 0; dx < sxLen; ++dx) {
									const int lx = (cx0 - localX) + dx;
									const int idx = (lz << 6) | (ly << 3) | lx;
									const uint64_t bit = (uint64_t)1u << ((ly << 3) | lx);
									const bool wasSet = (mask[lz] & bit) != 0u;
									const bool nowSet = (SparseRawVolumeStorage::voxelBits(srcRow[dx]) != defaultBits);
									tile[idx].overwrite(srcRow[dx]);
									if (nowSet && !wasSet) {
										mask[lz] |= bit;
										++count;
									} else if (!nowSet && wasSet) {
										mask[lz] &= ~bit;
										--count;
									}
								}
							}
						}
					}

					SparseRawVolumeStorage::PreparedChunkBrick item;
					item.chunkIndex = _storage->chunkIndexAt(localX, localY, localZ);
					item.brick = SparseChunk::prepareBrick(localX & SparseRawVolumeStorage::ChunkMask,
														   localY & SparseRawVolumeStorage::ChunkMask,
														   localZ & SparseRawVolumeStorage::ChunkMask, tile, mask,
														   count);
					localPrepared.push_back(item);
				}
			}
		}
		if (!localPrepared.empty()) {
			core::ScopedLock scoped(mergeLock);
			allPrepared.reserve(allPrepared.size() + localPrepared.size());
			for (const SparseRawVolumeStorage::PreparedChunkBrick &item : localPrepared) {
				allPrepared.push_back(item);
			}
		}
	});
	_storage->adoptPreparedBricks(allPrepared);
	return true;
}

RawVolume::RawVolume(RawVolume &&move) noexcept {
	_data = move._data;
	move._data = nullptr;
	_storage = move._storage;
	move._storage = nullptr;
	_region = move._region;
	_borderVoxel = move._borderVoxel;
}

RawVolume::RawVolume(const Voxel *data, const voxel::Region &region) {
	initialise(region, true);
	if (_data != nullptr && data != nullptr) {
		const size_t bytes = RawVolume::size(_region);
		core_memcpy((void *)_data, (const void *)data, bytes);
	}
}

RawVolume::RawVolume(Voxel *data, const voxel::Region &region) : _region(region), _data(data), _storage(nullptr) {
	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");
}

RawVolume::~RawVolume() {
	core_free(_data);
	_data = nullptr;
	delete _storage;
	_storage = nullptr;
}

bool RawVolume::move(const glm::ivec3 &shift) {
	if (!valid()) {
		return false;
	}
	if (_storage != nullptr) {
		return _storage->move(shift);
	}
	const int w = width();
	const int h = height();
	const int d = depth();

	glm::ivec3 t = shift;
	t.x = (t.x % w + w) % w;
	t.y = (t.y % h + h) % h;
	t.z = (t.z % d + d) % d;

	const int64_t hwstride = (int64_t)h * w;
	for (int z = 0; z < d; ++z) {
		const int64_t zhwstride = (int64_t)z * hwstride;
		for (int y = 0; y < h; ++y) {
			const int64_t begin = zhwstride + (int64_t)y * w;
			core::rotate(_data + begin, _data + begin + t.x, _data + zhwstride + (int64_t)(y + 1) * w);
		}
	}

	const int64_t yoffset = (int64_t)t.y * w;
	for (int z = 0; z < d; ++z) {
		core::rotate(_data + (int64_t)z * hwstride, _data + (int64_t)z * hwstride + yoffset,
					 _data + (int64_t)(z + 1) * hwstride);
	}

	core::rotate(_data, _data + (int64_t)t.z * hwstride, _data + (int64_t)d * hwstride);

	return true;
}

Voxel *RawVolume::copyVoxels() const {
	if (!valid()) {
		return nullptr;
	}
	const size_t bytes = RawVolume::size(_region);
	if (bytes == 0u) {
		return nullptr;
	}
	Voxel *rawCopy = (Voxel *)core_malloc(bytes);
	if (rawCopy == nullptr) {
		return nullptr;
	}
	if (_storage != nullptr) {
		_storage->copyTo(rawCopy);
	} else {
		core_memcpy((void *)rawCopy, (const void *)_data, bytes);
	}
	return rawCopy;
}

Voxel *RawVolume::voxels() const {
	if (_data != nullptr) {
		return _data;
	}
	if (_storage == nullptr) {
		return nullptr;
	}
	const size_t bytes = RawVolume::size(_region);
	if (bytes == 0u) {
		return nullptr;
	}
	Voxel *dense = (Voxel *)core_malloc(bytes);
	if (dense == nullptr) {
		Log::error("Failed to materialize %" SDL_PRIu64 " bytes for sparse volume", (uint64_t)bytes);
		return nullptr;
	}
	_storage->copyTo(dense);
	delete _storage;
	_storage = nullptr;
	_data = dense;
	return _data;
}

bool RawVolume::isSparseAirDefault() const {
	return _storage != nullptr && isAir(_storage->defaultVoxel().getMaterial());
}

Voxel RawVolume::sparseDefaultVoxel() const {
	return _storage != nullptr ? _storage->defaultVoxel() : Voxel();
}

bool RawVolume::hasOccupiedVoxelsInRegion(const Region &region) const {
	if (!valid() || !intersects(_region, region)) {
		return false;
	}
	if (_storage == nullptr) {
		return true;
	}
	Region r = region;
	r.cropTo(_region);
	const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)_region.getLowerX()),
						   (int)((int64_t)r.getLowerY() - (int64_t)_region.getLowerY()),
						   (int)((int64_t)r.getLowerZ() - (int64_t)_region.getLowerZ()));
	const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)_region.getLowerX()),
						   (int)((int64_t)r.getUpperY() - (int64_t)_region.getLowerY()),
						   (int)((int64_t)r.getUpperZ() - (int64_t)_region.getLowerZ()));
	return _storage->hasOccupiedVoxelsInBox(lMins, lMaxs);
}

const SparseChunk::Brick *RawVolume::sparseBrickAt(int localX, int localY, int localZ) const {
	return _storage != nullptr ? _storage->brickAt(localX, localY, localZ) : nullptr;
}

bool RawVolume::readSparseBrickTile(const glm::ivec3 &localBrickOrigin, Voxel (&outTile)[SparseChunk::BrickVoxels],
									uint64_t (&outMask)[SparseChunk::BrickMaskWords], int &outCount) const {
	core_assert(_storage != nullptr);
	return _storage->readBrickTile(localBrickOrigin.x, localBrickOrigin.y, localBrickOrigin.z, outTile, outMask,
								   outCount);
}

size_t RawVolume::sparseChunkIndex(const glm::ivec3 &localBrickOrigin) const {
	core_assert(_storage != nullptr);
	return _storage->chunkIndexAt(localBrickOrigin.x, localBrickOrigin.y, localBrickOrigin.z);
}

void RawVolume::adoptPreparedSparseBricks(core::DynamicArray<PreparedTileBrick> &prepared) {
	if (_storage == nullptr || prepared.empty()) {
		for (PreparedTileBrick &item : prepared) {
			item.brick.destroy();
		}
		prepared.clear();
		return;
	}
	core::DynamicArray<SparseRawVolumeStorage::PreparedChunkBrick> converted;
	converted.reserve(prepared.size());
	for (PreparedTileBrick &item : prepared) {
		SparseRawVolumeStorage::PreparedChunkBrick cb;
		cb.chunkIndex = item.chunkIndex;
		cb.brick = item.brick;
		item.brick.heapVoxels = nullptr;
		item.brick.count = 0;
		converted.push_back(cb);
	}
	prepared.clear();
	_storage->adoptPreparedBricks(converted);
}

void RawVolume::collectOccupiedSparseBricks(const Region &region, core::DynamicArray<OccupiedBrickSpan> &out) const {
	out.clear();
	if (_storage == nullptr || !intersects(_region, region)) {
		return;
	}
	Region r = region;
	r.cropTo(_region);
	const glm::ivec3 &lower = _region.getLowerCorner();
	const glm::ivec3 lMins((int)((int64_t)r.getLowerX() - (int64_t)lower.x),
						   (int)((int64_t)r.getLowerY() - (int64_t)lower.y),
						   (int)((int64_t)r.getLowerZ() - (int64_t)lower.z));
	const glm::ivec3 lMaxs((int)((int64_t)r.getUpperX() - (int64_t)lower.x),
						   (int)((int64_t)r.getUpperY() - (int64_t)lower.y),
						   (int)((int64_t)r.getUpperZ() - (int64_t)lower.z));
	core::DynamicArray<SparseRawVolumeStorage::OccupiedBrickInfo> infos;
	_storage->collectOccupiedBricks(lMins, lMaxs, infos);
	out.reserve(infos.size());
	for (const SparseRawVolumeStorage::OccupiedBrickInfo &info : infos) {
		out.push_back(OccupiedBrickSpan{glm::ivec3((int32_t)((int64_t)lower.x + info.localOrigin.x),
												   (int32_t)((int64_t)lower.y + info.localOrigin.y),
												   (int32_t)((int64_t)lower.z + info.localOrigin.z)),
										info.brick});
	}
}

void RawVolume::notifySparseModified() {
	if (_storage != nullptr) {
		_storage->notifyModified();
	}
}

const uint32_t *RawVolume::sparseGenerationPtr() const {
	return _storage != nullptr ? _storage->generationPtr() : nullptr;
}

void RawVolume::sampleSparseAt(int lx, int ly, int lz, const SparseChunk *&outChunk,
							   const SparseChunk::Brick *&outBrick, const Voxel *&outRow, bool &outWholeBrick,
							   glm::ivec3 &outLower, glm::ivec3 &outUpper, Voxel &outFallbackVoxel,
							   uint32_t &outGen) const {
	core_assert(_storage != nullptr);
	_storage->sampleAt(lx, ly, lz, outChunk, outBrick, outRow, outWholeBrick, outLower, outUpper, outFallbackVoxel,
					   outGen);
}

const Voxel &RawVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	if (_region.containsPoint(x, y, z)) {
		const int64_t lx = (int64_t)x - (int64_t)_region.getLowerX();
		const int64_t ly = (int64_t)y - (int64_t)_region.getLowerY();
		const int64_t lz = (int64_t)z - (int64_t)_region.getLowerZ();
		if (core_likely(_data != nullptr)) {
			return _data[lx + ly * width() + lz * _region.stride()];
		}
		if (_storage != nullptr) {
			return _storage->voxel((int)lx, (int)ly, (int)lz);
		}
	}
	return _borderVoxel;
}

void RawVolume::fillBehindPlane(math::Axis axis, int slice, int minA, int maxA, int minB, int maxB, Voxel *out) const {
	if (out == nullptr || !valid() || minA > maxA || minB > maxB ||
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

	const int aCount = maxA - minA + 1;
	const int bCount = maxB - minB + 1;
	const int cellCount = aCount * bCount;
	const int maxBehind = (int)((int64_t)slice - (int64_t)_region.getLowerCorner()[dAxis]);
	if (maxBehind <= 0) {
		return;
	}

	int remaining = 0;
	for (int i = 0; i < cellCount; ++i) {
		if (isAir(out[i].getMaterial())) {
			++remaining;
		}
	}
	if (remaining == 0) {
		return;
	}

	if (_storage != nullptr) {
		const glm::ivec3 &lower = _region.getLowerCorner();
		// Fast path: if default is air and the entire behind-region is empty, skip immediately
		if (isAir(_storage->defaultVoxel().getMaterial())) {
			glm::ivec3 behindLo = lo;
			glm::ivec3 behindHi = hi;
			behindLo[dAxis] = lower[dAxis];
			behindHi[dAxis] = slice - 1;
			if (isEmpty(Region(behindLo, behindHi))) {
				return;
			}
		}
		const int localStartD = (int)((int64_t)slice - (int64_t)lower[dAxis]);
		const int localMinA = (int)((int64_t)minA - (int64_t)lower[aAxis]);
		const int localMinB = (int)((int64_t)minB - (int64_t)lower[bAxis]);
		for (int step = 1; step <= maxBehind && remaining > 0; ++step) {
			const int dCoord = localStartD - step;
			int i = 0;
			for (int b = 0; b < bCount && remaining > 0; ++b) {
				const int bCoord = localMinB + b;
				for (int a = 0; a < aCount; ++a, ++i) {
					if (!isAir(out[i].getMaterial())) {
						continue;
					}
					int coords[3];
					coords[dAxis] = dCoord;
					coords[aAxis] = localMinA + a;
					coords[bAxis] = bCoord;
					const Voxel &v = _storage->voxel(coords[0], coords[1], coords[2]);
					if (!isAir(v.getMaterial())) {
						out[i] = v;
						--remaining;
					}
				}
			}
		}
		return;
	}

	const int64_t strides[3] = {1, (int64_t)width(), _region.stride()};
	const int64_t startIndex = _region.index(lo);

	if (axis == math::Axis::X) {
		const int64_t aStride = strides[aAxis];
		const int64_t bStride = strides[bAxis];
		int i = 0;
		for (int b = 0; b < bCount && remaining > 0; ++b) {
			const Voxel *column = _data + startIndex + (int64_t)b * bStride;
			for (int a = 0; a < aCount; ++a, ++i, column += aStride) {
				if (!isAir(out[i].getMaterial())) {
					continue;
				}
				const Voxel *current = column;
				int left = maxBehind;
				while (left-- > 0) {
					--current;
					if (!isAir(current->getMaterial())) {
						out[i] = *current;
						--remaining;
						break;
					}
				}
			}
		}
		return;
	}

	const int64_t aStride = strides[aAxis];
	const int64_t bStride = strides[bAxis];
	const int64_t dStride = strides[dAxis];
	for (int step = 1; step <= maxBehind && remaining > 0; ++step) {
		int64_t plane = startIndex - (int64_t)step * dStride;
		int i = 0;
		for (int b = 0; b < bCount && remaining > 0; ++b) {
			int64_t row = plane + (int64_t)b * bStride;
			for (int a = 0; a < aCount; ++a, ++i, row += aStride) {
				if (!isAir(out[i].getMaterial())) {
					continue;
				}
				const Voxel &voxel = _data[row];
				if (!isAir(voxel.getMaterial())) {
					out[i] = voxel;
					--remaining;
				}
			}
		}
	}
}

void RawVolume::setBorderValue(const Voxel &voxel) {
	_borderVoxel = voxel;
}

bool RawVolume::setVoxel(int32_t x, int32_t y, int32_t z, const Voxel &voxel) {
	return setVoxel(glm::ivec3(x, y, z), voxel);
}

bool RawVolume::setVoxel(int64_t idx, const Voxel &voxel) {
	if (!valid() || idx < 0 || idx >= _region.stride() * depth()) {
		return false;
	}
	if (core_likely(_data != nullptr)) {
		if (_data[idx] == voxel) {
			return false;
		}
		_data[idx] = voxel;
		return true;
	}
	const int64_t s = _region.stride();
	const int lz = (int)(idx / s);
	const int64_t rem = idx - (int64_t)lz * s;
	const int ly = (int)(rem / width());
	const int lx = (int)(rem - (int64_t)ly * width());
	return _storage->setVoxel(lx, ly, lz, voxel);
}

bool RawVolume::setVoxel(const glm::ivec3 &pos, const Voxel &voxel) {
	const bool inside = _region.containsPoint(pos);
	core_assert_msg(inside, "Position is outside valid region %i:%i:%i (mins[%i:%i:%i], maxs[%i:%i:%i])", pos.x, pos.y,
					pos.z, _region.getLowerX(), _region.getLowerY(), _region.getLowerZ(), _region.getUpperX(),
					_region.getUpperY(), _region.getUpperZ());
	if (!inside || !valid()) {
		return false;
	}
	const int64_t lx = (int64_t)pos.x - (int64_t)_region.getLowerX();
	const int64_t ly = (int64_t)pos.y - (int64_t)_region.getLowerY();
	const int64_t lz = (int64_t)pos.z - (int64_t)_region.getLowerZ();
	if (core_likely(_data != nullptr)) {
		const int64_t index = lx + ly * width() + lz * _region.stride();
		if (_data[index] == voxel) {
			return false;
		}
		_data[index] = voxel;
		return true;
	}
	return _storage->setVoxel((int)lx, (int)ly, (int)lz, voxel);
}

bool RawVolume::setVoxelOverwrite(int32_t x, int32_t y, int32_t z, const Voxel &voxel) {
	if (!_region.containsPoint(x, y, z) || !valid()) {
		return false;
	}
	const int64_t lx = (int64_t)x - (int64_t)_region.getLowerX();
	const int64_t ly = (int64_t)y - (int64_t)_region.getLowerY();
	const int64_t lz = (int64_t)z - (int64_t)_region.getLowerZ();
	if (core_likely(_data != nullptr)) {
		const int64_t index = lx + ly * width() + lz * _region.stride();
		if (SparseRawVolumeStorage::voxelBits(_data[index]) == SparseRawVolumeStorage::voxelBits(voxel)) {
			return false;
		}
		_data[index].overwrite(voxel);
		return true;
	}
	return _storage->setVoxelOverwrite((int)lx, (int)ly, (int)lz, voxel);
}

void RawVolume::setVoxelsRow(int32_t x, int32_t y, int32_t z, int32_t count, const Voxel &voxel) {
	if (count <= 0 || !valid()) {
		return;
	}
	if (!_region.containsPointInY(y) || !_region.containsPointInZ(z)) {
		return;
	}
	const int64_t x0 = core_max((int64_t)x, (int64_t)_region.getLowerX());
	const int64_t x1 = core_min((int64_t)x + (int64_t)count - 1, (int64_t)_region.getUpperX());
	if (x0 > x1) {
		return;
	}
	const int32_t clippedCount = (int32_t)(x1 - x0 + 1);
	const int64_t lx = x0 - (int64_t)_region.getLowerX();
	const int64_t ly = (int64_t)y - (int64_t)_region.getLowerY();
	const int64_t lz = (int64_t)z - (int64_t)_region.getLowerZ();
	if (_data != nullptr) {
		Voxel *dst = _data + lx + ly * width() + lz * _region.stride();
		for (int32_t i = 0; i < clippedCount; ++i) {
			dst[i] = voxel;
		}
		return;
	}
	_storage->setVoxelsRow((int)lx, (int)ly, (int)lz, clippedCount, voxel);
}

void RawVolume::initialise(const Region &regValidRegion, bool forceDense) {
	_region = regValidRegion;

	core_assert_msg(width() > 0, "Volume width must be greater than zero.");
	core_assert_msg(height() > 0, "Volume height must be greater than zero.");
	core_assert_msg(depth() > 0, "Volume depth must be greater than zero.");

	if (!forceDense && shouldUseSparseStorage(_region)) {
		_storage = new SparseRawVolumeStorage(_region.getDimensionsInVoxels());
		if (!_storage->valid()) {
			delete _storage;
			_storage = nullptr;
			Log::error("Failed to allocate sparse storage for volume %i:%i:%i", width(), height(), depth());
		}
		return;
	}

	const size_t bytes = RawVolume::size(_region);
	if (bytes == 0u) {
		Log::error("Volume byte size overflow for dimensions %i:%i:%i", width(), height(), depth());
		return;
	}
	_data = (Voxel *)core_calloc(1, bytes);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for a volume with the dimensions %i:%i:%i",
				   (uint64_t)bytes, width(), height(), depth());
		return;
	}
}

void RawVolume::clear() {
	Voxel voxel;
	fill(voxel);
}

void RawVolume::fill(const voxel::Voxel &voxel) {
	if (!_region.isValid() || !valid()) {
		return;
	}
	if (_storage != nullptr) {
		_storage->fill(voxel);
		return;
	}
	const size_t count = (size_t)_region.stride() * (size_t)depth();
	uint32_t val;
	core_memcpy(&val, &voxel, sizeof(val));
	core_memset4((void *)_data, val, count);
	static_assert(sizeof(Voxel) == sizeof(uint32_t), "Voxel is expected to be 4 bytes");
}

bool RawVolume::valid() const {
	return _data != nullptr || (_storage != nullptr && _storage->valid());
}

size_t RawVolume::allocatedBytes() const {
	if (_storage != nullptr) {
		return _storage->allocatedBytes();
	}
	return _data != nullptr ? size(_region) : 0u;
}

} // namespace voxel
