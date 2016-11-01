/**
 * @file
 */

#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"

namespace voxel {

class GeneratorContext;

class WorldPersister {
public:
	bool load(GeneratorContext& ctx, long seed);
	bool save(GeneratorContext& ctx, long seed);
	void erase(GeneratorContext& ctx, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
