/**
 * @file
 */
#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include "voxel/WorldPersister.h"
#include "voxel/Constants.h"
#include "core/Trace.h"
#include "core/Log.h"
#include "noise/Noise.h"

namespace voxel {

class BiomeManager;
class PagedVolumeWrapper;

constexpr int WORLDGEN_TREES = 1 << 0;
constexpr int WORLDGEN_CLOUDS = 1 << 1;

constexpr int WORLDGEN_CLIENT = WORLDGEN_TREES | WORLDGEN_CLOUDS;
constexpr int WORLDGEN_SERVER = WORLDGEN_TREES;

/**
 * @brief Pager implementation for PagedVolume.
 */
class WorldPager: public PagedVolume::Pager {
private:
	struct WorldContext {
		WorldContext();
		bool load(const std::string& lua);

		int landscapeNoiseOctaves;
		float landscapeNoiseLacunarity;
		float landscapeNoiseFrequency;
		float landscapeNoiseGain;

		int caveNoiseOctaves;
		float caveNoiseLacunarity;
		float caveNoiseFrequency;
		float caveNoiseGain;
		float caveDensityThreshold;

		int mountainNoiseOctaves;
		float mountainNoiseLacunarity;
		float mountainNoiseFrequency;
		float mountainNoiseGain;
	};

	WorldPersister _worldPersister;
	long _seed = 0l;
	int _createFlags = 0;
	glm::vec2 _noiseSeedOffset;

	PagedVolume *_volumeData = nullptr;
	BiomeManager* _biomeManager = nullptr;
	WorldContext _ctx;
	noise::Noise _noise;

	// don't access the volume in anything that is called here
	void create(PagedVolume::PagerContext& ctx);

	// use a 2d noise to switch between different noises - to generate steep mountains
	template<class Volume>
	void createWorld(const WorldContext& worldCtx, Volume& volume, int noiseSeedOffsetX, int noiseSeedOffsetZ) const {
		core_trace_scoped(WorldGeneration);
		const Region& region = volume.region();
		Log::debug("Create new chunk at %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
		const int width = region.getWidthInVoxels();
		const int depth = region.getDepthInVoxels();
		const int lowerX = region.getLowerX();
		const int lowerY = region.getLowerY();
		const int lowerZ = region.getLowerZ();
		core_assert(region.getLowerY() >= 0);
		Voxel voxels[MAX_TERRAIN_HEIGHT];

		// TODO: store voxel data in local buffer and transfer in one step into the volume to reduce locking
		const int size = 2;
		core_assert(depth % size == 0);
		core_assert(width % size == 0);
		for (int z = lowerZ; z < lowerZ + depth; z += size) {
			for (int x = lowerX; x < lowerX + width; x += size) {
				const int ni = fillVoxels(x, lowerY, z, worldCtx, voxels, noiseSeedOffsetX, noiseSeedOffsetZ, MAX_TERRAIN_HEIGHT - 1);
				volume.setVoxels(x, lowerY, z, size, size, voxels, ni);
			}
		}
	}

	int fillVoxels(int x, int y, int z, const WorldContext& worldCtx, Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const;
	float getHeight(const glm::vec2& noisePos2d, const WorldContext& worldCtx) const;

public:
	/**
	 * @brief Initializes the pager
	 * @param volumeData The volume data to operate on
	 * @param biomeManager The already initialized BiomeManager
	 * @return @c true if initialization was successful, @c false otherwise
	 * @sa shutdown()
	 */
	bool init(PagedVolume *volumeData, BiomeManager* biomeManager, const std::string& worldParamsLua);
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

	void erase(const Region& region);
	/**
	 * @return @c true if the chunk was modified (created), @c false if it was just loaded
	 */
	bool pageIn(PagedVolume::PagerContext& ctx) override;
	void pageOut(PagedVolume::Chunk* chunk) override;
};

}
