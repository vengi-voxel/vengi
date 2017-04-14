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

	PagedVolumeWrapper(PagedVolume* voxelStorage, PagedVolume::ChunkPtr chunk, const Region& region);

	operator PagedVolume& () const;
	operator const PagedVolume& () const;
	operator PagedVolume* () const;
	operator const PagedVolume* () const;

	PagedVolume::ChunkPtr getChunk() const;
	PagedVolume* getVolume() const;
	const Region& getRegion() const;

	const Voxel& getVoxel(const glm::ivec3& pos) const;
	const Voxel& getVoxel(int x, int y, int z) const;

	bool setVoxel(const glm::ivec3& pos, const Voxel& voxel);
	bool setVoxel(int x, int y, int z, const Voxel& voxel);
	bool setVoxels(int x, int z, const Voxel* voxels, int amount);
	bool setVoxels(int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount);
	bool setVoxels(int x, int y, int z, const Voxel* voxels, int amount);
};

inline const Region& PagedVolumeWrapper::getRegion() const {
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

inline PagedVolume::ChunkPtr PagedVolumeWrapper::getChunk() const {
	return _chunk;
}

inline PagedVolume* PagedVolumeWrapper::getVolume() const {
	return _pagedVolume;
}

inline bool PagedVolumeWrapper::setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
	return setVoxel(pos.x, pos.y, pos.z, voxel);
}

inline const Voxel& PagedVolumeWrapper::getVoxel(const glm::ivec3& pos) const {
	return getVoxel(pos.x, pos.y, pos.z);
}

inline bool PagedVolumeWrapper::setVoxels(int x, int z, const Voxel* voxels, int amount) {
	return setVoxels(x, 0, z, 1, 1, voxels, amount);
}

inline bool PagedVolumeWrapper::setVoxels(int x, int y, int z, const Voxel* voxels, int amount) {
	return setVoxels(x, y, z, 1, 1, voxels, amount);
}

}
