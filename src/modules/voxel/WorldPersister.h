/**
 * @file
 */

#pragma once

#include <string>

namespace voxel {

class PagedVolumeWrapper;
class Region;

class WorldPersister {
public:
	bool load(PagedVolumeWrapper& ctx, long seed);
	bool save(PagedVolumeWrapper& ctx, long seed);
	void erase(PagedVolumeWrapper& ctx, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

}
