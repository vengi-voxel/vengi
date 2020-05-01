/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "voxel/PagedVolume.h"
#include "voxel/Region.h"
#include "core/Zip.h"
#include "core/ByteStream.h"
#include <memory>

namespace voxelworld {

class ChunkPersister : public core::IComponent {
public:
	virtual ~ChunkPersister() {}

	virtual bool init() override { return true; };
	virtual void shutdown() override { };

	virtual bool load(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) { return false; }
	virtual bool save(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) { return false; }
	virtual void erase(const voxel::Region& region, unsigned int seed) { }

	bool loadCompressed(const voxel::PagedVolume::ChunkPtr& chunk, const uint8_t *fileBuf, size_t fileLen) const;
	bool saveCompressed(const voxel::PagedVolume::ChunkPtr& chunk, core::ByteStream& outStream) const;
};

typedef std::shared_ptr<ChunkPersister> ChunkPersisterPtr;

}
