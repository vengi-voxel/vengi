/**
 * @file
 */

#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include <string>

namespace voxel {

class PagedVolumeWrapper;

class WorldPersister {
public:
	bool load(PagedVolumeWrapper& ctx, long seed);
	bool save(PagedVolume::Chunk* chunk, long seed);
	void erase(const Region& region, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
