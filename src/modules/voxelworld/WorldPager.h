/**
 * @file
 */
#pragma once

#include "voxel/PagedVolume.h"
#include "WorldContext.h"
#include "voxelformat/VolumeCache.h"
#include "voxel/Constants.h"
#include "core/Trace.h"
#include "core/Log.h"
#include "noise/Noise.h"
#include "BiomeManager.h"
#include <memory>
#include "ChunkPersister.h"
#include "TreeVolumeCache.h"

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
	unsigned int _seed = 0l;
	glm::vec2 _noiseSeedOffset;

	voxel::PagedVolume *_volumeData = nullptr;
	BiomeManager _biomeManager;
	WorldContext _worldCtx;
	noise::Noise _noise;
	TreeVolumeCache _volumeCache;
	ChunkPersisterPtr _chunkPersister;

	void createWorld(voxel::PagedVolumeWrapper& volume) const;
	void placeTrees(voxel::PagedVolume::PagerContext& pagerCtx);
	void addVolumeToPosition(voxel::PagedVolumeWrapper& target, const voxel::RawVolume* source, const glm::ivec3& pos);

	int terrainHeight(int x, int minsY, int z) const;
	int terrainHeight(int x, int minsY, int z, float n) const;
	int fillVoxels(int x, int minsY, int z, voxel::Voxel* voxels) const;

	/**
	 * @return A float value between [0.0-1.0]
	 */
	float getNoiseValue(float x, float z) const;
	float getDensity(float x, float y, float z, float n) const;

public:
	WorldPager(const voxelformat::VolumeCachePtr& volumeCache, const ChunkPersisterPtr& chunkPersister);
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

	const ChunkPersisterPtr& chunkPersister() const;

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

inline const ChunkPersisterPtr& WorldPager::chunkPersister() const {
	return _chunkPersister;
}

typedef std::shared_ptr<WorldPager> WorldPagerPtr;

}
