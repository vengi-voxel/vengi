/**
 * @file
 */
#include "WorldPager.h"
#include "voxel/BiomeManager.h"
#include "voxel/WorldContext.h"
#include "voxel/generator/WorldGenerator.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"

namespace voxel {

void WorldPager::erase(const Region& region) {
	_worldPersister.erase(region, _seed);
}

bool WorldPager::pageIn(PagedVolume::PagerContext& pctx) {
	core_assert(_volumeData != nullptr && _biomeManager != nullptr);
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	if (_worldPersister.load(pctx.chunk.get(), _seed)) {
		return false;
	}
	create(pctx);
	return true;
}

void WorldPager::pageOut(PagedVolume::Chunk* chunk) {
	core_assert(chunk != nullptr);
	_worldPersister.save(chunk, _seed);
}

void WorldPager::setPersist(bool persist) {
	_worldPersister.setPersist(persist);
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

bool WorldPager::init(PagedVolume *volumeData, BiomeManager* biomeManager, const std::string& worldParamsLua) {
	if (!_ctx.load(worldParamsLua)) {
		return false;
	}
	_volumeData = volumeData;
	_biomeManager = biomeManager;
	return _volumeData != nullptr && _biomeManager != nullptr;
}

void WorldPager::shutdown() {
	if (_volumeData != nullptr) {
		_volumeData->flushAll();
	}
	_volumeData = nullptr;
	_biomeManager = nullptr;
	_ctx = WorldContext();
}

void WorldPager::create(PagedVolume::PagerContext& ctx) {
	PagedVolumeWrapper wrapper(_volumeData, ctx.chunk, ctx.region);
	core_trace_scoped(CreateWorld);
	voxel::world::WorldGenerator gen(*_biomeManager, _seed);
	{
		core_trace_scoped(World);
		gen.createWorld(_ctx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
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
