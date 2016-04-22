#pragma once

#include "polyvox/Region.h"
#include "WorldContext.h"

namespace voxel {

class WorldPersister {
public:
	bool load(TerrainContext& ctx, long seed);
	bool save(TerrainContext& ctx, long seed);

	std::string getWorldName(const Region& region, long seed) const;
};

}
