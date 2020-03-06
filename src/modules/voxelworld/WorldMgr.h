/**
 * @file
 * @defgroup Voxel
 * @{
 * @}
 */

#pragma once

#include "voxel/PagedVolume.h"
#include "voxelutil/Raycast.h"
#include "voxelformat/VolumeCache.h"
#include "voxel/Constants.h"
#include <memory>

#include "core/GLM.h"
#include "math/Random.h"

namespace voxelworld {

/**
 * @brief The WorldMgr class is responsible to maintaining the voxel volumes and handle the needed mesh extraction
 * @ingroup Voxel
 */
class WorldMgr {
public:
	WorldMgr(const voxel::PagedVolume::PagerPtr& pager);
	~WorldMgr();

	/**
	 * @return true if the ray hit something - false if not.
	 * @note The callback has a parameter of @c const PagedVolume::Sampler& and returns a boolean. If the callback returns false,
	 * the ray is interrupted. Only if the callback returned false at some point in time, this function will return @c true.
	 */
	template<typename Callback>
	inline bool raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance, Callback&& callback) const {
		const voxel::RaycastResults::RaycastResult result = voxel::raycastWithDirection(_volumeData, start, direction * maxDistance, std::forward<Callback>(callback));
		return result == voxel::RaycastResults::Interupted;
	}

	template<typename VoxelTypeChecker>
	int findFloor(int x, int z, VoxelTypeChecker&& check) const {
		const glm::vec3 start(x, voxel::MAX_HEIGHT, z);
		const float distance = (float)voxel::MAX_HEIGHT;
		int y = voxel::NO_FLOOR_FOUND;
		raycast(start, glm::down, distance, [&] (const voxel::PagedVolume::Sampler& sampler) {
			if (check(sampler.voxel().getMaterial())) {
				y = sampler.position().y;
				return false;
			}
			return true;
		});
		return y;
	}

	/**
	 * @return The y component for the given x and z coordinates that is walkable - or @c NO_FLOOR_FOUND.
	 */
	int findWalkableFloor(const glm::vec3& position, float maxDistanceY = (float)voxel::MAX_HEIGHT) const;

	bool init(uint32_t volumeMemoryMegaBytes = 512, uint16_t chunkSideLength = 256);
	void shutdown();
	void reset();

	voxel::VoxelType material(int x, int y, int z) const;

	/**
	 * @brief Returns a random position inside the boundaries of the world (on the surface)
	 */
	glm::ivec3 randomPos() const;

	unsigned int seed() const;

	void setSeed(unsigned int seed);

	bool created() const;

	voxel::PagedVolume *volumeData();

	int chunkSize() const;

private:
	friend class WorldMgrTest;

	/**
	 * @brief Cuts the given world coordinate down to chunk tile vectors
	 */
	glm::ivec3 chunkPos(const glm::ivec3& pos) const;

	voxel::PagedVolume::PagerPtr _pager;
	voxel::PagedVolume *_volumeData = nullptr;
	mutable std::mt19937 _engine;
	long _seed = 0l;

	math::Random _random;
};

inline voxel::PagedVolume *WorldMgr::volumeData() {
	return _volumeData;
}

inline glm::ivec3 WorldMgr::chunkPos(const glm::ivec3& pos) const {
	const float size = chunkSize();
	const int x = glm::floor(pos.x / size);
	const int y = glm::floor(pos.y / size);
	const int z = glm::floor(pos.z / size);
	return glm::ivec3(x, y, z);
}

inline bool WorldMgr::created() const {
	return _seed != 0;
}

inline unsigned int WorldMgr::seed() const {
	return _seed;
}

typedef std::shared_ptr<WorldMgr> WorldMgrPtr;

}
