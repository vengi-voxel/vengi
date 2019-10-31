/**
 * @file
 */

#pragma once

#include "voxel/PagedVolume.h"
#include <string>

namespace voxel {
class PagedVolumeWrapper;
}

namespace voxelworld {

class WorldPersister {
protected:
	bool _persist = true;

public:
	void setPersist(bool persist);

	bool load(voxel::PagedVolume::Chunk* chunk, long seed);
	bool save(voxel::PagedVolume::Chunk* chunk, long seed);
	void erase(const voxel::Region& region, long seed);
	std::string getWorldName(const voxel::Region& region, long seed) const;
};

inline void WorldPersister::setPersist(bool persist) {
	_persist = persist;
}

}
