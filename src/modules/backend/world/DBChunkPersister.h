/**
 * @file
 */

#pragma once

#include "voxelworld/ChunkPersister.h"
#include "persistence/DBHandler.h"
#include "persistence/Blob.h"
#include "voxel/PagedVolume.h"
#include "voxel/Region.h"
#include "MapId.h"

namespace backend {

class DBChunkPersister : public voxelworld::ChunkPersister {
protected:
	persistence::DBHandlerPtr _dbHandler;
	const MapId _mapId;
public:
	DBChunkPersister(const persistence::DBHandlerPtr& dbHandler, MapId mapId);
	virtual ~DBChunkPersister() {}

	bool init() override;

	persistence::Blob load(int x, int y, int z, MapId mapId, unsigned int seed) const;
	/**
	 * @brief Removes all persisted chunks from the database for the given parameters
	 */
	bool truncate(unsigned int seed);

	bool load(voxel::PagedVolume::Chunk* chunk, unsigned int seed) override;
	bool save(voxel::PagedVolume::Chunk* chunk, unsigned int seed) override;
	void erase(const voxel::Region& region, unsigned int seed) override;
};

typedef std::shared_ptr<DBChunkPersister> DBChunkPersisterPtr;

}
