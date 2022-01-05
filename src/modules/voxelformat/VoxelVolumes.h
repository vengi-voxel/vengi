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

/**
 * @brief Struct that holds the metadata and the volume
 * @sa VoxelVolumes
 */
struct VoxelVolume {
	VoxelVolume(voxel::RawVolume* _volume = nullptr, const core::String& _name = "", bool _visible = true);
	VoxelVolume(voxel::RawVolume* _volume, const core::String& _name, bool _visible, const glm::ivec3& _pivot);
	VoxelVolume(VoxelVolume&& move) noexcept;
	VoxelVolume &operator=(VoxelVolume &&str) noexcept;

	voxel::RawVolume* volume = nullptr;
	core::String name;
	bool visible;
	glm::ivec3 pivot;

	void release();
};

/**
 * @brief The internal format for the save/load methods.
 * @note Does not free the attached volumes!
 *
 * @sa ScopedVoxelVolumes
 * @sa VoxelVolume
 * @sa clearVolumes()
 */
struct VoxelVolumes {
	core::DynamicArray<VoxelVolume> volumes;

	~VoxelVolumes();

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

/**
 * @param volumes VoxelVolumes instance to clean up - this does free the allocated memory of the volumes
 */
extern void clearVolumes(VoxelVolumes& volumes);

/**
 * @brief Using this class will automatically free the allocated memory of the volumes once the scope
 * was left.
 * @sa VoxelVolumes
 * @sa clearVolumes()
 */
struct ScopedVoxelVolumes : public VoxelVolumes {
	~ScopedVoxelVolumes() {
		clearVolumes(*this);
	}
};

}
