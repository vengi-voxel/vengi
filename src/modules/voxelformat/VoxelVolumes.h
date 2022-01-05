/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxel {

class RawVolume;

static constexpr int MaxRegionSize = 256;

/**
 * @brief Struct that holds the metadata and the volume
 * @sa VoxelVolumes
 */
class VoxelVolume {
public:
	VoxelVolume() {}
	VoxelVolume(voxel::RawVolume* _volume, const core::String& _name = "", bool _visible = true);
	VoxelVolume(voxel::RawVolume* _volume, const core::String& _name, bool _visible, const glm::ivec3& _pivot);
	VoxelVolume(const voxel::RawVolume* _volume, const core::String& _name = "", bool _visible = true);
	VoxelVolume(const voxel::RawVolume* _volume, const core::String& _name, bool _visible, const glm::ivec3& _pivot);
	VoxelVolume(VoxelVolume&& move) noexcept;
	VoxelVolume &operator=(VoxelVolume &&str) noexcept;

protected:
	core::String _name;
	voxel::RawVolume* _volume = nullptr;
	/**
	 * this will ensure that we are releasing the volume memory in this instance
	 * @sa release()
	 */
	bool _volumeOwned = true;
	bool _visible = true;
	glm::ivec3 _pivot { 0 };

public:
	/**
	 * @brief Releases the memory of the volume instance (only if owned).
	 */
	void release();

	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume() const;
	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume();
	/**
	 * @return voxel::Region instance that is invalid when the volume is not set for this instance.
	 */
	const voxel::Region &region() const;
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @param transferOwnership this is @c true if the volume should get deleted by this class, @c false if
	 * you are going to manage the instance on your own.
	 */
	void setVolume(voxel::RawVolume *volume, bool transferOwnership = true);
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @param transferOwnership this is @c true if the volume should get deleted by this class, @c false if
	 * you are going to manage the instance on your own.
	 */
	void setVolume(const voxel::RawVolume *volume, bool transferOwnership = true);

	// meta data

	const core::String &name() const;
	void setName(const core::String &name);
	bool visible() const;
	void setVisible(bool visible);
	const glm::ivec3 &pivot() const;
	void setPivot(const glm::ivec3 &pivot);
};

inline voxel::RawVolume* VoxelVolume::volume() const {
	return _volume;
}

inline voxel::RawVolume* VoxelVolume::volume() {
	return _volume;
}

inline const core::String &VoxelVolume::name() const {
	return _name;
}

inline void VoxelVolume::setName(const core::String &name) {
	_name = name;
}

inline bool VoxelVolume::visible() const {
	return _visible;
}

inline void VoxelVolume::setVisible(bool visible) {
	_visible = visible;
}

inline const glm::ivec3 &VoxelVolume::pivot() const {
	return _pivot;
}

inline void VoxelVolume::setPivot(const glm::ivec3& pivot) {
	_pivot = pivot;
}

/**
 * @brief The internal format for the save/load methods.
 * @note Does not free the attached volumes!
 *
 * @sa ScopedVoxelVolumes
 * @sa VoxelVolume
 * @sa clearVolumes()
 */
class VoxelVolumes {
protected:
	core::DynamicArray<VoxelVolume> _volumes;

public:
	~VoxelVolumes();

	void push_back(VoxelVolume&& v);
	void resize(size_t size);
	void reserve(size_t size);
	bool empty() const;
	size_t size() const;
	void release(int index);
	voxel::RawVolume* merge() const;

	void clear() {
		for (VoxelVolume& v : _volumes) {
			v.release();
		}
		_volumes.clear();
	}

	const VoxelVolume &operator[](size_t idx) const;
	VoxelVolume& operator[](size_t idx);

	inline auto begin() {
		return _volumes.begin();
	}

	inline auto end() {
		return _volumes.end();
	}

	inline auto begin() const {
		return _volumes.begin();
	}

	inline auto end() const {
		return _volumes.end();
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
		clear();
	}
};

}
