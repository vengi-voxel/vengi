/**
 * @file
 */
#include "WorldPager.h"
#include "math/Random.h"
#include "voxelworld/BiomeManager.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/Raycast.h"
#include "commonlua/LUA.h"
#include "core/String.h"
#include <array>

namespace voxelworld {

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

void WorldPager::erase(const voxel::Region& region) {
	_worldPersister.erase(region, _seed);
}

bool WorldPager::pageIn(voxel::PagedVolume::PagerContext& pctx) {
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

void WorldPager::pageOut(voxel::PagedVolume::Chunk* chunk) {
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

bool WorldPager::init(voxel::PagedVolume *volumeData, BiomeManager* biomeManager, const std::string& worldParamsLua) {
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
void WorldPager::createWorld(const WorldContext& worldCtx, voxel::PagedVolumeWrapper& volume, int noiseSeedOffsetX, int noiseSeedOffsetZ) const {
	core_trace_scoped(WorldGeneration);
	const voxel::Region& region = volume.region();
	Log::debug("Create new chunk at %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerY = region.getLowerY();
	const int lowerZ = region.getLowerZ();
	core_assert(region.getLowerY() >= 0);
	voxel::Voxel voxels[voxel::MAX_TERRAIN_HEIGHT];

	// TODO: store voxel data in local buffer and transfer in one step into the volume to reduce locking
	const int size = 2;
	core_assert(depth % size == 0);
	core_assert(width % size == 0);
	for (int z = lowerZ; z < lowerZ + depth; z += size) {
		for (int x = lowerX; x < lowerX + width; x += size) {
			const int ni = fillVoxels(x, lowerY, z, worldCtx, voxels, noiseSeedOffsetX, noiseSeedOffsetZ, voxel::MAX_TERRAIN_HEIGHT - 1);
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

int WorldPager::fillVoxels(int x, int lowerY, int z, const WorldContext& worldCtx, voxel::Voxel* voxels, int noiseSeedOffsetX, int noiseSeedOffsetZ, int maxHeight) const {
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

	const voxel::Voxel& water = createColorVoxel(voxel::VoxelType::Water, _seed);
	const voxel::Voxel& dirt = createColorVoxel(voxel::VoxelType::Dirt, _seed);
	static constexpr voxel::Voxel air;

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
			const voxel::Voxel& voxel = _biomeManager->getVoxel(pos, cave);
			voxels[y] = voxel;
		} else {
			if (y < voxel::MAX_WATER_HEIGHT) {
				voxels[y] = water;
			} else {
				voxels[y] = air;
			}
		}
	}
	for (int i = lowerY; i < voxel::MAX_WATER_HEIGHT; ++i) {
		if (voxels[i] == air) {
			voxels[i] = water;
		}
	}
	return core_max(ni - lowerY, voxel::MAX_WATER_HEIGHT - lowerY);
}

/**
 * @brief Looks for a suitable height level for placing a tree
 * @return @c -1 if no suitable floor for placing a tree was found
 */
static int findFloor(const voxel::PagedVolume* volume, int x, int z) {
	glm::ivec3 start(x, voxel::MAX_TERRAIN_HEIGHT - 1, z);
	glm::ivec3 end(x, voxel::MAX_WATER_HEIGHT, z);
	int y = voxel::NO_FLOOR_FOUND;
	voxel::raycastWithEndpoints(volume, start, end, [&y] (const typename voxel::PagedVolume::Sampler& sampler) {
		const voxel::Voxel& voxel = sampler.voxel();
		const voxel::VoxelType material = voxel.getMaterial();
		if (isLeaves(material)) {
			return false;
		}
		if (!isRock(material) && (isFloor(material) || isWood(material))) {
			y = sampler.position().y + 1;
			return false;
		}
		return true;
	});
	return y;
}

void WorldPager::create(voxel::PagedVolume::PagerContext& ctx) {
	voxel::PagedVolumeWrapper wrapper(_volumeData, ctx.chunk, ctx.region);
	core_trace_scoped(CreateWorld);
	math::Random random(_seed);
	createWorld(_ctx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
	placeTrees(ctx);
}

void WorldPager::placeTrees(voxel::PagedVolume::PagerContext& ctx) {
	// expand region to all surrounding regions by half of the region size.
	// we do this to be able to limit the generation on the current chunk. Otherwise
	// we would endlessly generate new chunks just because the trees overlap to
	// another chunk.
	const glm::ivec3& mins = ctx.region.getLowerCorner();
	const glm::ivec3& maxs = ctx.region.getUpperCorner();
	const glm::ivec3& dim = ctx.region.getDimensionsInVoxels();
	const std::array<voxel::Region, 9> regions = {
		// left neightbors
		voxel::Region(mins.x - dim.x, mins.y, mins.z - dim.z, maxs.x - dim.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x - dim.x, mins.y, mins.z,         maxs.x - dim.x, maxs.y, maxs.z        ),
		voxel::Region(mins.x - dim.x, mins.y, mins.z + dim.z, maxs.x - dim.x, maxs.y, maxs.z + dim.z),

		// right neightbors
		voxel::Region(mins.x + dim.x, mins.y, mins.z - dim.z, maxs.x + dim.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x + dim.x, mins.y, mins.z,         maxs.x + dim.x, maxs.y, maxs.z        ),
		voxel::Region(mins.x + dim.x, mins.y, mins.z + dim.z, maxs.x + dim.x, maxs.y, maxs.z + dim.z),

		// front and back neightbors
		voxel::Region(mins.x, mins.y, mins.z - dim.z, maxs.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x, mins.y, mins.z + dim.z, maxs.x, maxs.y, maxs.z + dim.z),

		// own chunk region
		voxel::Region(mins, maxs)
	};
	// the assumption here is that we get a full heigt paging request, otherwise we
	// would have to loop over more regions.
	core_assert(ctx.region.getLowerY() == 0);
	core_assert(ctx.region.getUpperY() == voxel::MAX_HEIGHT);
	voxel::PagedVolumeWrapper wrapper(_volumeData, ctx.chunk, ctx.region);
	std::vector<const char*> treeTypes;

	for (const voxel::Region& region : regions) {
		treeTypes = _biomeManager->getTreeTypes(region);
		if (treeTypes.empty()) {
			Log::debug("No tree types given for region %s", region.toString().c_str());
			return;
		}
		const int border = 2;
		math::Random random(_seed + region.getCentreX() + region.getCentreZ());

		random.shuffle(treeTypes.begin(), treeTypes.end());

		std::vector<glm::vec2> positions;
		_biomeManager->getTreePositions(region, positions, random, border);
		Log::debug("Found %i possible positions", (int)positions.size());

		int treeTypeIndex = 0;
		const int treeTypeSize = (int)treeTypes.size();
		for (const glm::vec2& position : positions) {
			glm::ivec3 treePos(position.x, 0, position.y);
			if (!_volumeData->hasChunk(treePos)) {
				continue;
			}
			treePos.y = findFloor(_volumeData, position.x, position.y);
			if (treePos.y <= voxel::MAX_WATER_HEIGHT) {
				continue;
			}
			const char *treeType = treeTypes[treeTypeIndex++];
			treeTypeIndex %= treeTypeSize;
			// TODO: this hardcoded 10... no way
			const int treeIndex = random.random(0, 10);
			char filename[64];
			if (!core::string::formatBuf(filename, sizeof(filename), "models/trees/%s/%i.vox", treeType, treeIndex)) {
				Log::error("Failed to assemble tree path");
				continue;
			}
			const voxel::RawVolume* v = nullptr; // TODO
			if (v == nullptr) {
				continue;
			}
			addVolumeToPosition(wrapper, v, treePos);
		}
	}
}

void WorldPager::addVolumeToPosition(voxel::PagedVolumeWrapper& target, const voxel::RawVolume* source, const glm::ivec3& pos) {
	const voxel::Region& region = source->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int z = mins.z; z <= maxs.z; ++z) {
				target.setVoxel(x + pos.x, y + pos.y, z + pos.z, source->voxel(x, y, z));
			}
		}
	}
}

}
