/**
 * @file
 */

#pragma once

#include "PagedVolume.h"

namespace voxel {

/**
 * @brief Wrapper around a PagedVolume that reduces the amount of needed locks.
 */
class PagedVolumeWrapper {
private:
	PagedVolume* _pagedVolume;
	PagedVolume::ChunkPtr _chunk;
	Region _validRegion;
	Region _region;
public:
	class Sampler : public PagedVolume::Sampler {
	private:
		using Super = PagedVolume::Sampler;
		PagedVolume::ChunkPtr _chunk;
	public:
		Sampler(const PagedVolumeWrapper* volume);
		Sampler(const PagedVolumeWrapper& volume);

		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos) override;
	};

	PagedVolumeWrapper(PagedVolume* voxelStorage, const PagedVolume::ChunkPtr& chunk, const Region& region);
	virtual ~PagedVolumeWrapper() {}

	operator PagedVolume& () const;
	operator const PagedVolume& () const;
	operator PagedVolume* () const;
	operator const PagedVolume* () const;

	PagedVolume::ChunkPtr chunk() const;
	PagedVolume* volume() const;
	const Region& region() const;

	const Voxel& voxel(const glm::ivec3& pos) const;
	const Voxel& voxel(int x, int y, int z) const;

	bool setVoxel(const glm::ivec3& pos, const Voxel& voxel);
	bool setVoxel(int x, int y, int z, const Voxel& voxel);
	bool setVoxels(int x, int z, const Voxel* voxels, int amount);
	bool setVoxels(int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount);
	bool setVoxels(int x, int y, int z, const Voxel* voxels, int amount);
};

inline const Region& PagedVolumeWrapper::region() const {
	return _region;
}

inline PagedVolumeWrapper::operator PagedVolume& () const {
	return *_pagedVolume;
}

inline PagedVolumeWrapper::operator const PagedVolume& () const {
	return *_pagedVolume;
}

inline PagedVolumeWrapper::operator PagedVolume* () const {
	return _pagedVolume;
}

inline PagedVolumeWrapper::operator const PagedVolume* () const {
	return _pagedVolume;
}

inline PagedVolume::ChunkPtr PagedVolumeWrapper::chunk() const {
	return _chunk;
}

inline PagedVolume* PagedVolumeWrapper::volume() const {
	return _pagedVolume;
}

inline bool PagedVolumeWrapper::setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
	return setVoxel(pos.x, pos.y, pos.z, voxel);
}

inline const Voxel& PagedVolumeWrapper::voxel(const glm::ivec3& pos) const {
	return voxel(pos.x, pos.y, pos.z);
}

inline bool PagedVolumeWrapper::setVoxels(int x, int z, const Voxel* voxels, int amount) {
	return setVoxels(x, 0, z, 1, 1, voxels, amount);
}

inline bool PagedVolumeWrapper::setVoxels(int x, int y, int z, const Voxel* voxels, int amount) {
	return setVoxels(x, y, z, 1, 1, voxels, amount);
}

}
