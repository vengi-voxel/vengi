/**
 * @file
 */

#include "DBChunkPersister.h"
#include "BackendModels.h"
#include "voxel/PagedVolume.h"
#include "voxel/Region.h"

namespace backend {

DBChunkPersister::DBChunkPersister(const persistence::DBHandlerPtr &dbHandler, MapId mapId) :
		_dbHandler(dbHandler), _mapId(mapId) {
}

bool DBChunkPersister::init() {
	if (!_dbHandler->createTable(db::ChunkModel())) {
		return false;
	}
	return true;
}

void DBChunkPersister::erase(const voxel::Region& region, unsigned int seed) {
	db::ChunkModel model;
	model.setMapid(_mapId);
	model.setX(region.getLowerX());
	model.setY(region.getLowerY());
	model.setZ(region.getLowerZ());
	model.setSeed(seed);
	_dbHandler->deleteModel(model);
}

bool DBChunkPersister::truncate(unsigned int seed) {
	db::ChunkModel model;
	model.setMapid(_mapId);
	model.setSeed(seed);
	return _dbHandler->truncate(model);
}

persistence::Blob DBChunkPersister::load(int x, int y, int z, MapId mapId, unsigned int seed) const {
	db::ChunkModel model;
	model.setMapid(mapId);
	model.setX(x);
	model.setY(y);
	model.setZ(z);
	model.setSeed(seed);
	if (!_dbHandler->select(model, persistence::DBConditionOne())) {
		Log::warn("Failed to load the model");
	}
	return model.data();
}

bool DBChunkPersister::load(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) {
	core_trace_scoped(DBChunkPersisterLoad);
	const glm::ivec3& region = chunk->chunkPos();
	persistence::Blob blob = load(region.x, region.y, region.z, _mapId, seed);
	if (blob.length <= 0) {
		Log::debug("No chunk found in database");
		blob.release();
		return false;
	}
	if (!loadCompressed(chunk, blob.data, blob.length)) {
		Log::warn("Failed to uncompress the model");
		blob.release();
		return false;
	}
	blob.release();
	return true;
}

// TODO: this must be done async
bool DBChunkPersister::save(const voxel::PagedVolume::ChunkPtr& chunk, unsigned int seed) {
	core_trace_scoped(DBChunkPersisterSave);
	core::ByteStream out;
	if (!saveCompressed(chunk, out)) {
		return false;
	}

	persistence::Blob data;
	data.data = (uint8_t*)out.getBuffer();
	data.length = out.getSize();
	Log::info("Store compressed chunk with size %i", (int)data.length);
	db::ChunkModel model;
	model.setMapid(_mapId);
	const glm::ivec3& chunkPos = chunk->chunkPos();
	model.setX(chunkPos.x);
	model.setY(chunkPos.y);
	model.setZ(chunkPos.z);
	model.setSeed(seed);
	model.setData(data);
	return _dbHandler->insert(model);
}

}
