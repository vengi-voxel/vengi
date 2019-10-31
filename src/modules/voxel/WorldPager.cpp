/**
 * @file
 */
#include "WorldPager.h"
#include "math/Random.h"
#include "voxel/BiomeManager.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "commonlua/LUA.h"

namespace voxel {

#define CTX_LUA_FLOAT(name) name = lua.floatValue(#name, name)
#define CTX_LUA_INT(name) name = lua.intValue(#name, name)

WorldPager::WorldContext::WorldContext() :
	landscapeNoiseOctaves(1), landscapeNoiseLacunarity(0.1f), landscapeNoiseFrequency(0.005f), landscapeNoiseGain(0.6f),
	caveNoiseOctaves(1), caveNoiseLacunarity(0.1f), caveNoiseFrequency(0.05f), caveNoiseGain(0.1f), caveDensityThreshold(0.83f),
	mountainNoiseOctaves(2), mountainNoiseLacunarity(0.3f), mountainNoiseFrequency(0.00075f), mountainNoiseGain(0.5f) {
}

bool WorldPager::WorldContext::load(const std::string& luaString) {
	if (luaString.empty()) {
		return true;
	}
	lua::LUA lua;
	if (!lua.load(luaString)) {
		Log::error("Could not load lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}

	CTX_LUA_INT(landscapeNoiseOctaves);
	CTX_LUA_FLOAT(landscapeNoiseLacunarity);
	CTX_LUA_FLOAT(landscapeNoiseFrequency);
	CTX_LUA_FLOAT(landscapeNoiseGain);
	CTX_LUA_INT(caveNoiseOctaves);
	CTX_LUA_FLOAT(caveNoiseLacunarity);
	CTX_LUA_FLOAT(caveNoiseFrequency);
	CTX_LUA_FLOAT(caveNoiseGain);
	CTX_LUA_FLOAT(caveDensityThreshold);
	CTX_LUA_INT(mountainNoiseOctaves);
	CTX_LUA_FLOAT(mountainNoiseLacunarity);
	CTX_LUA_FLOAT(mountainNoiseFrequency);
	CTX_LUA_FLOAT(mountainNoiseGain);

	return true;
}

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

void WorldPager::setNoiseOffset(const glm::vec2& noiseOffset) {
	_noiseSeedOffset = noiseOffset;
}

bool WorldPager::init(PagedVolume *volumeData, BiomeManager* biomeManager, const std::string& worldParamsLua) {
	if (!_ctx.load(worldParamsLua)) {
		return false;
	}
	if (!_noise.init()) {
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
	_noise.shutdown();
	_volumeData = nullptr;
	_biomeManager = nullptr;
	_ctx = WorldContext();
}

// use a 2d noise to switch between different noises - to generate steep mountains
void WorldPager::createWorld(const WorldContext& worldCtx, PagedVolumeWrapper& volume, int noiseSeedOffsetX, int noiseSeedOffsetZ) const {
	core_trace_scoped(WorldGeneration);
	const Region& region = volume.region();
	Log::debug("Create new chunk at %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerY = region.getLowerY();
	const int lowerZ = region.getLowerZ();
	core_assert(region.getLowerY() >= 0);
	Voxel voxels[MAX_TERRAIN_HEIGHT];

	// TODO: store voxel data in local buffer and transfer in one step into the volume to reduce locking
	const int size = 2;
	core_assert(depth % size == 0);
	core_assert(width % size == 0);
	for (int z = lowerZ; z < lowerZ + depth; z += size) {
		for (int x = lowerX; x < lowerX + width; x += size) {
			const int ni = fillVoxels(x, lowerY, z, worldCtx, voxels, noiseSeedOffsetX, noiseSeedOffsetZ, MAX_TERRAIN_HEIGHT - 1);
			volume.setVoxels(x, lowerY, z, size, size, voxels, ni);
		}
	}
}

float WorldPager::getHeight(const glm::vec2& noisePos2d, const WorldContext& worldCtx) const {
	// TODO: move the noise settings into the biome
	const float landscapeNoise = noise::fBm(noisePos2d * worldCtx.landscapeNoiseFrequency, worldCtx.landscapeNoiseOctaves,
			worldCtx.landscapeNoiseLacunarity, worldCtx.landscapeNoiseGain);
	const float noiseNormalized = noise::norm(landscapeNoise);
	const float mountainNoise = noise::fBm(noisePos2d * worldCtx.mountainNoiseFrequency, worldCtx.mountainNoiseOctaves,
			worldCtx.mountainNoiseLacunarity, worldCtx.mountainNoiseGain);
	const float mountainNoiseNormalized = noise::norm(mountainNoise);
	const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
	const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
	return n;
}

int WorldPager::fillVoxels(int x, int lowerY, int z, const WorldContext& worldCtx, Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const {
	const glm::vec2 noisePos2d(noiseSeedOffsetX + x, noiseSeedOffsetZ + z);
	const float n = getHeight(noisePos2d, worldCtx);
	const glm::ivec3 noisePos3d(x, lowerY, z);
	int centerHeight;
	const float cityMultiplier = _biomeManager->getCityMultiplier(glm::ivec2(x, z), &centerHeight);
	int ni = n * maxHeight;
	if (cityMultiplier < 1.0f) {
		const float revn = (1.0f - cityMultiplier);
		ni = revn * centerHeight + (n * maxHeight * cityMultiplier);
	}
	if (ni < lowerY) {
		return 0;
	}

	const Voxel& water = createColorVoxel(VoxelType::Water, _seed);
	const Voxel& dirt = createColorVoxel(VoxelType::Dirt, _seed);
	static constexpr Voxel air;

	voxels[0] = dirt;
	glm::ivec3 pos(x, 0, z);
	for (int y = ni - 1; y >= lowerY + 1; --y) {
		const glm::vec3 noisePos3d(noisePos2d.x, y, noisePos2d.y);
		// TODO: move the noise settings into the biome
		const float noiseVal = noise::norm(
				noise::fBm(noisePos3d * worldCtx.caveNoiseFrequency, worldCtx.caveNoiseOctaves, worldCtx.caveNoiseLacunarity, worldCtx.caveNoiseGain));
		const float finalDensity = n + noiseVal;
		if (finalDensity > worldCtx.caveDensityThreshold) {
			const bool cave = y < ni - 1;
			pos.y = y;
			const Voxel& voxel = _biomeManager->getVoxel(pos, cave);
			voxels[y] = voxel;
		} else {
			if (y < MAX_WATER_HEIGHT) {
				voxels[y] = water;
			} else {
				voxels[y] = air;
			}
		}
	}
	for (int i = lowerY; i < MAX_WATER_HEIGHT; ++i) {
		if (voxels[i] == air) {
			voxels[i] = water;
		}
	}
	return core_max(ni - lowerY, MAX_WATER_HEIGHT - lowerY);
}

void WorldPager::create(PagedVolume::PagerContext& ctx) {
	PagedVolumeWrapper wrapper(_volumeData, ctx.chunk, ctx.region);
	core_trace_scoped(CreateWorld);
	math::Random random(_seed);
	createWorld(_ctx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
}

}
