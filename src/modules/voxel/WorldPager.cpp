/**
 * @file
 */
#include "WorldPager.h"
#include "voxel/BiomeManager.h"
#include "voxel/WorldContext.h"
#include "voxel/generator/WorldGenerator.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"

#define PERSIST 1

namespace voxel {

void WorldPager::erase(const Region& region) {
#if PERSIST
	_worldPersister.erase(region, _seed);
#endif
}

bool WorldPager::pageIn(PagedVolume::PagerContext& pctx) {
	core_assert(_ctx != nullptr && _volumeData != nullptr && _biomeManager != nullptr);
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
#if PERSIST
	if (_persist && _worldPersister.load(pctx.chunk.get(), _seed)) {
		return false;
	}
#endif
	PagedVolumeWrapper ctx(_volumeData, pctx.chunk, pctx.region);
	create(ctx);
	return true;
}

void WorldPager::pageOut(PagedVolume::Chunk* chunk) {
#if PERSIST
	if (!_persist) {
		return;
	}
	core_assert(chunk != nullptr);
	_worldPersister.save(chunk, _seed);
#endif
}

void WorldPager::setPersist(bool persist) {
	_persist = persist;
}

void WorldPager::setSeed(long seed) {
	_seed = seed;
}

void WorldPager::setCreateFlags(int flags) {
	_createFlags = flags;
}

void WorldPager::setNoiseOffset(const glm::vec2& noiseOffset) {
	_noiseSeedOffset = noiseOffset;
}

bool WorldPager::init(PagedVolume *volumeData, BiomeManager* biomeManager, WorldContext* ctx) {
	_volumeData = volumeData;
	_biomeManager = biomeManager;
	_ctx = ctx;
	return _ctx != nullptr && _volumeData != nullptr && _biomeManager != nullptr;
}

void WorldPager::shutdown() {
	if (_volumeData != nullptr) {
		_volumeData->flushAll();
	}
	_volumeData = nullptr;
	_biomeManager = nullptr;
	_ctx = nullptr;
}

void WorldPager::create(PagedVolumeWrapper& wrapper) {
	core_trace_scoped(CreateWorld);
	voxel::world::WorldGenerator gen(*_biomeManager, _seed);
	{
		core_trace_scoped(World);
		gen.createWorld(*_ctx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
	}
	if ((_createFlags & voxel::world::WORLDGEN_CLOUDS) != 0) {
		core_trace_scoped(Clouds);
		voxel::cloud::CloudContext ctx;
		gen.createClouds(wrapper, ctx);
	}
	if ((_createFlags & voxel::world::WORLDGEN_TREES) != 0) {
		core_trace_scoped(Trees);
		gen.createTrees(wrapper);
	}
	{
		core_trace_scoped(Buildings);
		gen.createBuildings(wrapper);
	}
}

}
