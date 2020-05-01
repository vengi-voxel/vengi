/**
 * @file
 */

#pragma once

#include "ChunkPersister.h"

namespace voxel {
class PagedVolumeWrapper;
}

namespace voxelworld {

class FilePersister : public ChunkPersister {
public:
	virtual ~FilePersister() {}

	bool load(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) override;
	bool save(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) override;
	void erase(const voxel::Region& region, unsigned int seed) override;
};

}
