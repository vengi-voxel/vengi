/**
 * @file
 */

#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"

namespace voxel {

class TerrainContext;

class WorldPersister {
public:
	bool load(TerrainContext& ctx, long seed);
	bool save(TerrainContext& ctx, long seed);
	void erase(TerrainContext& ctx, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
