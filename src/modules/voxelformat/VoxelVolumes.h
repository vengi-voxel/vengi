/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/io/File.h"
#include "core/Common.h"
#include <vector>

namespace voxel {

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
	std::vector<VoxelVolume> volumes;

	~VoxelVolumes() ;

	inline void push_back(VoxelVolume&& v) {
		volumes.emplace_back(core::forward<VoxelVolume>(v));
	}

	inline void resize(size_t size) {
		volumes.resize(size);
	}

	inline void reserve(size_t size) {
		volumes.reserve(size);
	}

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

	inline bool empty() const {
		return volumes.empty();
	}

	inline size_t size() const {
		return volumes.size();
	}

	const VoxelVolume &operator[](size_t idx) const {
		return volumes[idx];
	}

	VoxelVolume& operator[](size_t idx) {
		return volumes[idx];
	}

	voxel::RawVolume* merge() const;
};

}