/**
 * @file
 */
#include "WorldPager.h"
#include "math/Random.h"
#include "voxel/BiomeManager.h"
#include "voxel/WorldContext.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/generator/CloudGenerator.h"
#include "voxel/generator/BuildingGenerator.h"
#include "voxel/generator/TreeGenerator.h"

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
	return std::max(ni - lowerY, MAX_WATER_HEIGHT - lowerY);
}

void WorldPager::create(PagedVolume::PagerContext& ctx) {
	PagedVolumeWrapper wrapper(_volumeData, ctx.chunk, ctx.region);
	core_trace_scoped(CreateWorld);
	math::Random random(_seed);
	{
		core_trace_scoped(World);
		createWorld(_ctx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
	}
	if ((_createFlags & WORLDGEN_CLOUDS) != 0) {
		core_trace_scoped(Clouds);
		voxel::cloud::CloudContext cloudCtx;
		const voxel::Region& region = wrapper.region();
		voxel::cloud::createClouds(wrapper, region, *_biomeManager, cloudCtx);
	}
	if ((_createFlags & WORLDGEN_TREES) != 0) {
		core_trace_scoped(Trees);
		const voxel::Region& region = wrapper.region();
		voxel::tree::createTrees(wrapper, region, *_biomeManager);
	}
	{
		core_trace_scoped(Buildings);
		const voxel::Region& region = wrapper.region();
		glm::ivec3 buildingPos = region.getCentre();
		if (_biomeManager->hasCity(buildingPos)) {
			for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; --i) {
				const VoxelType material = wrapper.voxel(buildingPos.x, i, buildingPos.z).getMaterial();
				if (!isFloor(material)) {
					continue;
				}
				buildingPos.y = i;
				if (random.fithyFifthy()) {
					voxel::building::createBuilding(wrapper, buildingPos, voxel::BuildingType::House);
				} else {
					voxel::building::createBuilding(wrapper, buildingPos, voxel::BuildingType::Tower);
				}
				break;
			}
		}
	}
}

}
