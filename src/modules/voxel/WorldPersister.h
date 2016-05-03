#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"

namespace voxel {

class TerrainContext;

class WorldPersister {
public:
	bool load(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	bool save(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	void erase(TerrainContext& ctx, PagedVolume::Chunk* chunk, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
