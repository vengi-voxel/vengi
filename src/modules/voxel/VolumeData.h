/**
 * @file
 */

#pragma once

#include "voxel/Region.h"

namespace voxel {

template<typename T>
class VolumeData {
private:
	T *_data;
	const voxel::Region _region;

	inline int64_t index(int x, int y, int z) const {
		const int64_t iLocalXPos = x - _region.getLowerX();
		const int64_t iLocalYPos = y - _region.getLowerY();
		const int64_t iLocalZPos = z - _region.getLowerZ();
		return iLocalXPos + (iLocalYPos * _region.getWidthInVoxels()) +
			   (iLocalZPos * _region.stride());
	}

public:
	using value_type = T;

	VolumeData(const voxel::Region &region, T defaultVal) : _region(region) {
		const int64_t size = _region.voxels();
		_data = (T *)core_malloc(size * sizeof(T));
		core_memset(_data, defaultVal, size * sizeof(T));
	}

	~VolumeData() {
		core_free(_data);
	}

	const voxel::Region &region() const {
		return _region;
	}

	void setValue(int x, int y, int z, const T &value) {
		const int64_t idx = index(x, y, z);
		if (idx < 0 || idx >= _region.voxels()) {
			return;
		}
		_data[idx] = value;
	}

	inline void setValue(const glm::ivec3 &pos, const T &value) {
		setValue(pos.x, pos.y, pos.z, value);
	}

	T value(int x, int y, int z) const {
		if (!_region.containsPoint(x, y, z)) {
			return 0u;
		}
		const int64_t idx = index(x, y, z);
		if (idx < 0 || idx >= _region.voxels()) {
			return 0u;
		}
		return _data[idx];
	}

	inline T value(const glm::ivec3 &pos) const {
		return value(pos.x, pos.y, pos.z);
	}
};

} // namespace voxel
