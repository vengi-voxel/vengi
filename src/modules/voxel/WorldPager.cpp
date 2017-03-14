/**
 * @file
 */
#include "WorldPager.h"
#include "voxel/BiomeManager.h"
#include "voxel/WorldContext.h"
#include "voxel/generator/WorldGenerator.h"

#define PERSIST 1

namespace voxel {

void WorldPager::erase(PagedVolume::PagerContext& pctx) {
#if PERSIST
	core_assert(_ctx != nullptr && _volumeData != nullptr && _biomeManager != nullptr);
	PagedVolumeWrapper ctx(_volumeData, pctx.chunk, pctx.region);
	_worldPersister.erase(ctx, _seed);
#endif
}

bool WorldPager::pageIn(PagedVolume::PagerContext& pctx) {
	core_assert(_ctx != nullptr && _volumeData != nullptr && _biomeManager != nullptr);
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	PagedVolumeWrapper ctx(_volumeData, pctx.chunk, pctx.region);
#if PERSIST
	if (_persist && _worldPersister.load(ctx, _seed)) {
		return false;
	}
#endif
	create(ctx);
	return true;
}

void WorldPager::pageOut(PagedVolume::PagerContext& pctx) {
#if PERSIST
	core_assert(_ctx != nullptr && _volumeData != nullptr && _biomeManager != nullptr);
	if (!_persist) {
		return;
	}
	PagedVolumeWrapper ctx(_volumeData, pctx.chunk, pctx.region);
	_worldPersister.save(ctx, _seed);
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
	gen.createWorld(*_ctx, wrapper, _createFlags, _noiseSeedOffset.x, _noiseSeedOffset.y);
}

}
