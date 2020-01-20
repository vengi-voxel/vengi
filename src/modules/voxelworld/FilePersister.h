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
protected:
	bool _persist;
public:
	virtual ~FilePersister() {}

	void setPersist(bool persist) {
		_persist = persist;
	}

	bool load(voxel::PagedVolume::Chunk* chunk, unsigned int seed) override;
	bool save(voxel::PagedVolume::Chunk* chunk, unsigned int seed) override;
	void erase(const voxel::Region& region, unsigned int seed) override;
};

}
