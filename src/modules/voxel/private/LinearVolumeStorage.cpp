/**
 * @file
 */

#include "LinearVolumeStorage.h"
#include "core/Algorithm.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include <SDL3/SDL_stdinc.h>
#include <glm/common.hpp>

namespace voxel {

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

size_t LinearVolumeStorage::byteSize() const {
	const size_t voxels = (size_t)_dimensions.x * (size_t)_dimensions.y * (size_t)_dimensions.z;
	core_assert_msg(voxels > 0 && voxels <= SIZE_MAX / sizeof(Voxel), "Invalid linear volume storage size");
	return voxels * sizeof(Voxel);
}

void LinearVolumeStorage::resetToPlaceholder() {
	_dimensions = glm::ivec3(1);
	_data = (Voxel *)core_malloc(sizeof(Voxel));
	core_assert_always(_data != nullptr);
	_owned = true;
	*_data = Voxel();
}

LinearVolumeStorage::LinearVolumeStorage(const glm::ivec3 &dimensions, bool clear)
	: _dimensions(dimensions), _owned(true) {
	core_assert(glm::all(glm::greaterThan(dimensions, glm::ivec3(0))));
	const size_t size = byteSize();
	_data = (Voxel *)core_malloc(size);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for linear volume storage %i:%i:%i", (uint64_t)size,
				   dimensions.x, dimensions.y, dimensions.z);
	}
	core_assert_always(_data != nullptr);
	if (clear) {
		fill(Voxel());
	}
}

LinearVolumeStorage::LinearVolumeStorage(Voxel *data, const glm::ivec3 &dimensions, bool owned)
	: _dimensions(dimensions), _data(data), _owned(owned) {
	core_assert(glm::all(glm::greaterThan(dimensions, glm::ivec3(0))));
	core_assert_msg(data != nullptr, "Linear storage buffer must not be null");
}

LinearVolumeStorage::LinearVolumeStorage(const LinearVolumeStorage &other)
	: _dimensions(other._dimensions), _owned(true) {
	const size_t size = other.byteSize();
	_data = (Voxel *)core_malloc(size);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for linear volume storage copy", (uint64_t)size);
	}
	core_assert_always(_data != nullptr);
	core_memcpy((void *)_data, (const void *)other._data, size);
}

LinearVolumeStorage::LinearVolumeStorage(LinearVolumeStorage &&other) noexcept
	: _dimensions(other._dimensions), _data(other._data), _owned(other._owned) {
	other.resetToPlaceholder();
}

LinearVolumeStorage &LinearVolumeStorage::operator=(const LinearVolumeStorage &other) {
	if (this == &other) {
		return *this;
	}
	LinearVolumeStorage copy(other);
	return *this = core::move(copy);
}

LinearVolumeStorage &LinearVolumeStorage::operator=(LinearVolumeStorage &&other) noexcept {
	if (this == &other) {
		return *this;
	}
	if (_owned) {
		core_free(_data);
	}
	_dimensions = other._dimensions;
	_data = other._data;
	_owned = other._owned;
	other.resetToPlaceholder();
	return *this;
}

LinearVolumeStorage::~LinearVolumeStorage() {
	if (_owned) {
		core_free(_data);
	}
}

void LinearVolumeStorage::fill(const Voxel &value) {
	const size_t voxels = (size_t)_dimensions.x * (size_t)_dimensions.y * (size_t)_dimensions.z;
	uint32_t pattern;
	core_memcpy(&pattern, &value, sizeof(pattern));
	core_memset4((void *)_data, pattern, voxels);
	static_assert(sizeof(Voxel) == sizeof(uint32_t), "Voxel is expected to be 4 bytes");
}

void LinearVolumeStorage::copyTo(Voxel *destination) const {
	core_assert(destination != nullptr);
	core_memcpy((void *)destination, (const void *)_data, byteSize());
}

void LinearVolumeStorage::copyFrom(const LinearVolumeStorage &src) {
	core_assert_msg(_dimensions == src._dimensions, "copyFrom requires matching dimensions");
	core_memcpy((void *)_data, (const void *)src._data, byteSize());
}

void LinearVolumeStorage::copyFrom(const Voxel *src) {
	core_assert(src != nullptr);
	core_memcpy((void *)_data, (const void *)src, byteSize());
}

void LinearVolumeStorage::copyFrom(const LinearVolumeStorage &src, const glm::ivec3 &srcMins, const glm::ivec3 &dstMins,
								   const glm::ivec3 &size) {
	core_assert(glm::all(glm::greaterThan(size, glm::ivec3(0))));
	core_assert(glm::all(glm::greaterThanEqual(srcMins, glm::ivec3(0))));
	core_assert(glm::all(glm::greaterThanEqual(dstMins, glm::ivec3(0))));
	core_assert(glm::all(glm::lessThanEqual(srcMins + size, src._dimensions)));
	core_assert(glm::all(glm::lessThanEqual(dstMins + size, _dimensions)));

	const int64_t srcYStride = src._dimensions.x;
	const int64_t srcZStride = src.stride();
	const int64_t dstYStride = _dimensions.x;
	const int64_t dstZStride = stride();
	const size_t lineSize = sizeof(Voxel) * (size_t)size.x;

	for (int z = 0; z < size.z; ++z) {
		const int64_t srcZBase = (int64_t)(srcMins.z + z) * srcZStride + srcMins.x;
		const int64_t dstZBase = (int64_t)(dstMins.z + z) * dstZStride + dstMins.x;
		for (int y = 0; y < size.y; ++y) {
			const Voxel *srcLine = src._data + srcZBase + (int64_t)(srcMins.y + y) * srcYStride;
			Voxel *dstLine = _data + dstZBase + (int64_t)(dstMins.y + y) * dstYStride;
			core_memcpy((void *)dstLine, (const void *)srcLine, lineSize);
		}
	}
}

bool LinearVolumeStorage::hasFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) const {
	const uint32_t flagsMask = (uint32_t)(flags & 0x3) << 2;
	const int width = _dimensions.x;
	const int height = _dimensions.y;
	const int scanWidth = localMaxs.x - localMins.x + 1;
	const int scanHeight = localMaxs.y - localMins.y + 1;
	const int scanDepth = localMaxs.z - localMins.z + 1;

	if (scanWidth == width && scanHeight == height) {
		const Voxel *start = _data + (int64_t)localMins.z * width * height;
		return scanFlagsLinear(start, (int64_t)scanDepth * width * height, flagsMask);
	}

	if (scanWidth == width) {
		const int64_t zStride = (int64_t)width * height;
		const int64_t rowsPerSlice = (int64_t)scanHeight * width;
		for (int z = 0; z < scanDepth; ++z) {
			const Voxel *start = _data + (int64_t)(localMins.z + z) * zStride + (int64_t)localMins.y * width;
			if (scanFlagsLinear(start, rowsPerSlice, flagsMask)) {
				return true;
			}
		}
		return false;
	}

	const int64_t yStride = width;
	const int64_t zStride = (int64_t)width * height;
	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		const int64_t zBase = (int64_t)z * zStride + localMins.x;
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			if (scanFlagsLinear(_data + zBase + (int64_t)y * yStride, scanWidth, flagsMask)) {
				return true;
			}
		}
	}
	return false;
}

bool LinearVolumeStorage::regionForFlag(uint8_t flag, glm::ivec3 &outLocalMins, glm::ivec3 &outLocalMaxs) const {
	const int w = _dimensions.x;
	const int h = _dimensions.y;
	const int d = _dimensions.z;
	const int64_t yStride = w;
	const int64_t zStride = (int64_t)w * h;
	const uint32_t flagsMask = (uint32_t)(flag & 0x3) << 2;

	int minZ = -1;
	for (int z = 0; z < d; ++z) {
		if (scanFlagsLinear(_data + (int64_t)z * zStride, zStride, flagsMask)) {
			minZ = z;
			break;
		}
	}
	if (minZ < 0) {
		return false;
	}

	int maxZ = minZ;
	for (int z = d - 1; z > minZ; --z) {
		if (scanFlagsLinear(_data + (int64_t)z * zStride, zStride, flagsMask)) {
			maxZ = z;
			break;
		}
	}

	int minY = h;
	int maxY = -1;
	int minX = w;
	int maxX = -1;

	for (int z = minZ; z <= maxZ; ++z) {
		const int64_t zBase = (int64_t)z * zStride;

		if (z != minZ && z != maxZ && !scanFlagsLinear(_data + zBase, zStride, flagsMask)) {
			continue;
		}

		for (int y = 0; y < minY; ++y) {
			const int64_t base = zBase + (int64_t)y * yStride;
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
			const int64_t base = zBase + (int64_t)y * yStride;
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
				const int64_t base = zBase + (int64_t)y * yStride;
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

	outLocalMins = glm::ivec3(minX, minY, minZ);
	outLocalMaxs = glm::ivec3(maxX, maxY, maxZ);
	return true;
}

void LinearVolumeStorage::applyFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags,
									 FlagOp op) {
	const uint32_t flagsMask32 = (uint32_t)(flags & 0x3) << 2;
	const uint64_t flagsMask64 = ((uint64_t)flagsMask32 << 32) | flagsMask32;
	const uint64_t clearMask64 = ~flagsMask64;
	const uint32_t clearMask32 = ~flagsMask32;

	const int64_t yStride = _dimensions.x;
	const int64_t zStride = stride();
	const int lineLength = localMaxs.x - localMins.x + 1;

	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		const int64_t zBase = (int64_t)z * zStride + localMins.x;
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			const int64_t baseIndex = zBase + (int64_t)y * yStride;
			int offset = 0;
			int remaining = lineLength;

			if ((baseIndex & 1) && remaining > 0) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex];
				if (op == FlagOp::Remove) {
					*data32 &= clearMask32;
				} else if (op == FlagOp::Toggle) {
					*data32 ^= flagsMask32;
				} else {
					*data32 |= flagsMask32;
				}
				offset = 1;
				remaining--;
			}

			const int pairs = remaining / 2;
			uint64_t *data64 = (uint64_t *)&_data[baseIndex + offset];
			for (int i = 0; i < pairs; ++i) {
				if (op == FlagOp::Remove) {
					data64[i] &= clearMask64;
				} else if (op == FlagOp::Toggle) {
					data64[i] ^= flagsMask64;
				} else {
					data64[i] |= flagsMask64;
				}
			}

			if (remaining & 1) {
				uint32_t *data32 = (uint32_t *)&_data[baseIndex + offset + pairs * 2];
				if (op == FlagOp::Remove) {
					*data32 &= clearMask32;
				} else if (op == FlagOp::Toggle) {
					*data32 ^= flagsMask32;
				} else {
					*data32 |= flagsMask32;
				}
			}
		}
	}
}

void LinearVolumeStorage::setFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) {
	applyFlags(localMins, localMaxs, flags, FlagOp::Set);
}

void LinearVolumeStorage::removeFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) {
	applyFlags(localMins, localMaxs, flags, FlagOp::Remove);
}

void LinearVolumeStorage::toggleFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) {
	applyFlags(localMins, localMaxs, flags, FlagOp::Toggle);
}

bool LinearVolumeStorage::isEmpty(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs) const {
	const int64_t yStride = _dimensions.x;
	const int64_t zStride = stride();
	const int lineLength = localMaxs.x - localMins.x + 1;
	const int voxelLineSize = (int)sizeof(Voxel) * lineLength;

	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		const int64_t zBase = (int64_t)z * zStride + localMins.x;
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			const Voxel *dataLine = _data + zBase + (int64_t)y * yStride;
			if (core::memchr_not(dataLine, 0, voxelLineSize) != nullptr) {
				return false;
			}
		}
	}
	return true;
}

void LinearVolumeStorage::fillBehindPlane(int dAxis, int aAxis, int bAxis, int localSlice, int minA, int maxA,
										  int minB, int maxB, Voxel *out) const {
	const int aCount = maxA - minA + 1;
	const int bCount = maxB - minB + 1;
	const int cellCount = aCount * bCount;
	const int maxBehind = localSlice;
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

	glm::ivec3 lo(0);
	lo[dAxis] = localSlice;
	lo[aAxis] = minA;
	lo[bAxis] = minB;
	const int64_t strides[3] = {1, (int64_t)_dimensions.x, stride()};
	const int64_t startIndex = index(lo);

	if (dAxis == 0) {
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
				const Voxel &value = _data[row];
				if (!isAir(value.getMaterial())) {
					out[i] = value;
					--remaining;
				}
			}
		}
	}
}

bool LinearVolumeStorage::move(const glm::ivec3 &shift) {
	const int w = _dimensions.x;
	const int h = _dimensions.y;
	const int d = _dimensions.z;

	glm::ivec3 t = shift;
	t.x = (t.x % w + w) % w;
	t.y = (t.y % h + h) % h;
	t.z = (t.z % d + d) % d;
	if (t == glm::ivec3(0)) {
		return true;
	}

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

size_t LinearVolumeStorage::allocatedBytes() const {
	return byteSize();
}

} // namespace voxel
