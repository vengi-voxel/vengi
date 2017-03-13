/**
 * @file
 */
#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/WorldPersister.h"

namespace voxel {

class BiomeManager;
struct WorldContext;

/**
 * @brief Pager implementation for PagedVolume.
 */
class WorldPager: public PagedVolume::Pager {
private:
	WorldPersister _worldPersister;
	bool _persist = true;
	long _seed = 0l;
	int _createFlags = 0;
	glm::vec2 _noiseSeedOffset;

	PagedVolume *_volumeData = nullptr;
	BiomeManager* _biomeManager = nullptr;
	WorldContext* _ctx = nullptr;

	// don't access the volume in anything that is called here
	void create(PagedVolumeWrapper& wrapper);

public:
	/**
	 * @brief Initializes the pager
	 * @param volumeData The volume data to operate on
	 * @param biomeManager The already initialized BiomeManager
	 * @param ctx The already initialized WorldContext
	 * @return @c true if initialization was successful, @c false otherwise
	 * @sa shutdown()
	 */
	bool init(PagedVolume *volumeData, BiomeManager* biomeManager, WorldContext* ctx);
	/**
	 * @brief Free resources and persist (if activated) the world data
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @brief Allow to switch whether you would like to persist the world data.
	 * @note Default is @c true
	 */
	void setPersist(bool persist);
	/**
	 * @brief The ssed that is going to be used for creating the world
	 */
	void setSeed(long seed);
	/**
	 * @li voxel::world::WORLDGEN_TREES
	 * @li voxel::world::WORLDGEN_CLOUDS
	 * @param flags Bitmask of world generator flags
	 */
	void setCreateFlags(int flags);

	void setNoiseOffset(const glm::vec2& noiseOffset);

	void erase(PagedVolume::PagerContext& ctx);
	/**
	 * @return @c true if the chunk was modified (created), @c false if it was just loaded
	 */
	bool pageIn(PagedVolume::PagerContext& ctx) override;
	void pageOut(PagedVolume::PagerContext& ctx) override;
};

}
