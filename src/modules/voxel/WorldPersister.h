#pragma once

#include "polyvox/Region.h"
#include "WorldContext.h"

namespace voxel {

class WorldPersister {
public:
	bool load(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	bool save(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	void erase(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
