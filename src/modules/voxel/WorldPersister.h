/**
 * @file
 */

#pragma once

#include "polyvox/Region.h"
#include "polyvox/PagedVolume.h"

namespace voxel {

class PagedVolumeWrapper;

class WorldPersister {
public:
	bool load(PagedVolumeWrapper& ctx, long seed);
	bool save(PagedVolumeWrapper& ctx, long seed);
	void erase(PagedVolumeWrapper& ctx, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
