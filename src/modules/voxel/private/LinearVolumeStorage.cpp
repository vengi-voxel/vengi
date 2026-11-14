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

size_t LinearVolumeStorage::byteSize() const {
	if (_dimensions.x <= 0 || _dimensions.y <= 0 || _dimensions.z <= 0) {
		return 0;
	}
	const size_t voxels = (size_t)_dimensions.x * (size_t)_dimensions.y * (size_t)_dimensions.z;
	if (voxels > SIZE_MAX / sizeof(Voxel)) {
		return 0;
	}
	return voxels * sizeof(Voxel);
}

LinearVolumeStorage::LinearVolumeStorage(const glm::ivec3 &dimensions) : _dimensions(dimensions), _owned(true) {
	core_assert(glm::all(glm::greaterThan(dimensions, glm::ivec3(0))));
	const size_t size = byteSize();
	if (size == 0) {
		Log::error("Invalid or oversized linear volume storage");
		return;
	}
	_data = (Voxel *)core_malloc(size);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for linear volume storage %i:%i:%i", (uint64_t)size,
				   dimensions.x, dimensions.y, dimensions.z);
		return;
	}
	fill(Voxel());
}

LinearVolumeStorage::LinearVolumeStorage(Voxel *data, const glm::ivec3 &dimensions, bool owned)
	: _dimensions(dimensions), _data(data), _owned(owned) {
	core_assert(glm::all(glm::greaterThan(dimensions, glm::ivec3(0))));
	core_assert_msg(data != nullptr || !owned, "Owned linear storage needs a buffer");
}

LinearVolumeStorage::LinearVolumeStorage(const LinearVolumeStorage &other)
	: _dimensions(other._dimensions), _owned(true) {
	const size_t size = other.byteSize();
	if (size == 0 || !other.valid()) {
		return;
	}
	_data = (Voxel *)core_malloc(size);
	if (_data == nullptr) {
		Log::error("Failed to allocate %" SDL_PRIu64 " bytes for linear volume storage copy", (uint64_t)size);
		return;
	}
	core_memcpy((void *)_data, (const void *)other._data, size);
}

LinearVolumeStorage::LinearVolumeStorage(LinearVolumeStorage &&other) noexcept
	: _dimensions(other._dimensions), _data(other._data), _owned(other._owned) {
	other._data = nullptr;
	other._owned = false;
	other._dimensions = glm::ivec3(0);
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
	other._data = nullptr;
	other._owned = false;
	other._dimensions = glm::ivec3(0);
	return *this;
}

LinearVolumeStorage::~LinearVolumeStorage() {
	if (_owned) {
		core_free(_data);
	}
	_data = nullptr;
}

bool LinearVolumeStorage::valid() const {
	return _data != nullptr;
}

const Voxel &LinearVolumeStorage::voxel(const glm::ivec3 &localPos) const {
	core_assert_msg(valid(), "Linear storage is invalid");
	return _data[index(localPos)];
}

bool LinearVolumeStorage::setVoxel(const glm::ivec3 &localPos, const Voxel &value) {
	Voxel *target = writableVoxel(localPos);
	if (target == nullptr) {
		return false;
	}
	if (*target == value) {
		return false;
	}
	*target = value;
	return true;
}

Voxel *LinearVolumeStorage::writableVoxel(const glm::ivec3 &localPos) {
	if (!valid()) {
		return nullptr;
	}
	return _data + index(localPos);
}

const Voxel *LinearVolumeStorage::row(const glm::ivec3 &localPos, int &count) const {
	if (!valid()) {
		count = 0;
		return nullptr;
	}
	count = _dimensions.x - localPos.x;
	return _data + index(localPos);
}

Voxel *LinearVolumeStorage::sampleData(const glm::ivec3 &localPos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper,
									   int64_t &yStride, int64_t &zStride) const {
	if (!valid()) {
		return nullptr;
	}
	dataLower = glm::ivec3(0);
	dataUpper = _dimensions - 1;
	yStride = _dimensions.x;
	zStride = stride();
	return _data + index(localPos);
}

void LinearVolumeStorage::fill(const Voxel &value) {
	if (!valid()) {
		return;
	}
	const size_t voxels = (size_t)_dimensions.x * (size_t)_dimensions.y * (size_t)_dimensions.z;
	uint32_t pattern;
	core_memcpy(&pattern, &value, sizeof(pattern));
	core_memset4((void *)_data, pattern, voxels);
	static_assert(sizeof(Voxel) == sizeof(uint32_t), "Voxel is expected to be 4 bytes");
}

void LinearVolumeStorage::copyTo(Voxel *destination) const {
	core_assert(destination != nullptr);
	if (!valid()) {
		return;
	}
	core_memcpy((void *)destination, (const void *)_data, byteSize());
}

bool LinearVolumeStorage::move(const glm::ivec3 &shift) {
	if (!valid()) {
		return false;
	}
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
	return valid() ? byteSize() : 0;
}

} // namespace voxel
