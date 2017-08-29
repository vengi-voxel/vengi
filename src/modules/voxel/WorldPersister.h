/**
 * @file
 */

#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include <string>

namespace voxel {

class PagedVolumeWrapper;

class WorldPersister {
protected:
	bool _persist = true;

public:
	void setPersist(bool persist);

	bool load(PagedVolume::Chunk* chunk, long seed);
	bool save(PagedVolume::Chunk* chunk, long seed);
	void erase(const Region& region, long seed);
	std::string getWorldName(const Region& region, long seed) const;
};

inline void WorldPersister::setPersist(bool persist) {
	_persist = persist;
}

}
