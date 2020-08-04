/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec3.hpp>

namespace voxel {

class RawVolume;

static constexpr int MaxRegionSize = 256;

struct VoxelVolume {
	VoxelVolume(RawVolume* _volume = nullptr, const core::String& _name = "", bool _visible = true);
	VoxelVolume(RawVolume* _volume, const core::String& _name, bool _visible, const glm::ivec3& _pivot);
	RawVolume* volume;
	core::String name;
	bool visible;
	glm::ivec3 pivot;
};

struct VoxelVolumes {
	core::DynamicArray<VoxelVolume> volumes;

	~VoxelVolumes() ;

	void push_back(VoxelVolume&& v);
	void resize(size_t size);
	void reserve(size_t size);
	bool empty() const;
	size_t size() const;
	voxel::RawVolume* merge() const;

	const VoxelVolume &operator[](size_t idx) const;
	VoxelVolume& operator[](size_t idx);

	inline auto begin() {
		return volumes.begin();
	}

	inline auto end() {
		return volumes.end();
	}

	inline auto begin() const {
		return volumes.begin();
	}

	inline auto end() const {
		return volumes.end();
	}
};

}
