/**
 * @file
 */
#pragma once

#include "voxel/PagedVolume.h"
#include "WorldPersister.h"
#include "WorldContext.h"
#include "voxelformat/VolumeCache.h"
#include "voxel/Constants.h"
#include "core/Trace.h"
#include "core/Log.h"
#include "noise/Noise.h"
#include "BiomeManager.h"
#include <memory>

namespace voxel {
class PagedVolumeWrapper;
class RawVolume;
}

namespace voxelworld {

/**
 * @brief Pager implementation for PagedVolume.
 *
 * This class is responsible for generating the voxel world.
 * The pager is the streaming interface for the voxel::PagedVolume.
 */
class WorldPager: public voxel::PagedVolume::Pager {
private:
	WorldPersister _worldPersister;
	unsigned int _seed = 0l;
	glm::vec2 _noiseSeedOffset;

	voxel::PagedVolume *_volumeData = nullptr;
	BiomeManager _biomeManager;
	WorldContext _worldCtx;
	noise::Noise _noise;
	voxelformat::VolumeCachePtr _volumeCache;

	// don't access the volume in anything that is called here
	void create(voxel::PagedVolume::PagerContext& pagerCtx);

	// use a 2d noise to switch between different noises - to generate steep mountains
	void createWorld(const WorldContext& worldCtx, voxel::PagedVolumeWrapper& volume, int noiseSeedOffsetX, int noiseSeedOffsetZ) const;
	void placeTrees(voxel::PagedVolume::PagerContext& pagerCtx);
	void addVolumeToPosition(voxel::PagedVolumeWrapper& target, const voxel::RawVolume* source, const glm::ivec3& pos);

	int fillVoxels(int x, int y, int z, const WorldContext& worldCtx, voxel::Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const;
	float getHeight(const glm::vec2& noisePos2d, const WorldContext& worldCtx) const;

public:
	WorldPager(const voxelformat::VolumeCachePtr& volumeCache);
	/**
	 * @brief Initializes the pager
	 * @param volumeData The volume data to operate on
	 * @return @c true if initialization was successful, @c false otherwise
	 * @sa shutdown()
	 */
	bool init(voxel::PagedVolume *volumeData, const std::string& worldParamsLua, const std::string& biomesLua);
	/**
	 * @brief Free resources and persist (if activated) the world data
	 * @sa init()
	 */
	void shutdown();
	void construct();

	const WorldPersister& worldPersister() const;

	/**
	 * @brief Allow to switch whether you would like to persist the world data.
	 * @note Default is @c true
	 */
	void setPersist(bool persist);
	/**
	 * @brief The ssed that is going to be used for creating the world
	 */
	void setSeed(unsigned int seed);

	void setNoiseOffset(const glm::vec2& noiseOffset);

	void erase(const voxel::Region& region);
	/**
	 * @return @c true if the chunk was modified (created), @c false if it was just loaded
	 */
	bool pageIn(voxel::PagedVolume::PagerContext& ctx) override;
	void pageOut(voxel::PagedVolume::Chunk* chunk) override;
};

inline const WorldPersister& WorldPager::worldPersister() const {
	return _worldPersister;
}

typedef std::shared_ptr<WorldPager> WorldPagerPtr;

}
