/**
 * @file
 */
#include "WorldPager.h"
#include "math/Random.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxelutil/Raycast.h"
#include "noise/Simplex.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/collection/Array.h"

namespace voxelworld {

WorldPager::WorldPager(const voxelformat::VolumeCachePtr& volumeCache) :
		_volumeCache(volumeCache) {
}

void WorldPager::erase(const voxel::Region& region) {
	_worldPersister.erase(region, _seed);
}

bool WorldPager::pageIn(voxel::PagedVolume::PagerContext& pctx) {
	core_assert(_volumeData != nullptr);
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

void WorldPager::setSeed(unsigned int seed) {
	_seed = seed;
}

void WorldPager::setNoiseOffset(const glm::vec2& noiseOffset) {
	_noiseSeedOffset = noiseOffset;
}

bool WorldPager::init(voxel::PagedVolume *volumeData, const std::string& worldParamsLua, const std::string& biomesLua) {
	if (!_biomeManager.init(biomesLua)) {
		Log::error("Failed to init biome mgr");
		return false;
	}
	if (!_worldCtx.load(worldParamsLua)) {
		return false;
	}
	if (!_noise.init()) {
		return false;
	}
	_volumeData = volumeData;
	return _volumeData != nullptr;
}

void WorldPager::shutdown() {
	if (_volumeData != nullptr) {
		_volumeData->flushAll();
	}
	_noise.shutdown();
	_volumeData = nullptr;
	_volumeCache = nullptr;
	_biomeManager.shutdown();
	_worldCtx = WorldContext();
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
	const float cityMultiplier = _biomeManager.getCityMultiplier(glm::ivec2(x, z), &centerHeight);
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
			const voxel::Voxel& voxel = _biomeManager.getVoxel(pos, cave);
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

void WorldPager::create(voxel::PagedVolume::PagerContext& pagerCtx) {
	voxel::PagedVolumeWrapper wrapper(_volumeData, pagerCtx.chunk, pagerCtx.region);
	core_trace_scoped(CreateWorld);
	math::Random random(_seed);
	createWorld(_worldCtx, wrapper, _noiseSeedOffset.x, _noiseSeedOffset.y);
	placeTrees(pagerCtx);
}

void WorldPager::placeTrees(voxel::PagedVolume::PagerContext& pagerCtx) {
	// expand region to all surrounding regions by half of the region size.
	// we do this to be able to limit the generation on the current chunk. Otherwise
	// we would endlessly generate new chunks just because the trees overlap to
	// another chunk.
	const glm::ivec3& mins = pagerCtx.region.getLowerCorner();
	const glm::ivec3& maxs = pagerCtx.region.getUpperCorner();
	const glm::ivec3& dim = pagerCtx.region.getDimensionsInVoxels();
	const core::Array<voxel::Region, 9> regions = {
		// left neighbors
		voxel::Region(mins.x - dim.x, mins.y, mins.z - dim.z, maxs.x - dim.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x - dim.x, mins.y, mins.z,         maxs.x - dim.x, maxs.y, maxs.z        ),
		voxel::Region(mins.x - dim.x, mins.y, mins.z + dim.z, maxs.x - dim.x, maxs.y, maxs.z + dim.z),

		// right neighbors
		voxel::Region(mins.x + dim.x, mins.y, mins.z - dim.z, maxs.x + dim.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x + dim.x, mins.y, mins.z,         maxs.x + dim.x, maxs.y, maxs.z        ),
		voxel::Region(mins.x + dim.x, mins.y, mins.z + dim.z, maxs.x + dim.x, maxs.y, maxs.z + dim.z),

		// front and back neighbors
		voxel::Region(mins.x, mins.y, mins.z - dim.z, maxs.x, maxs.y, maxs.z - dim.z),
		voxel::Region(mins.x, mins.y, mins.z + dim.z, maxs.x, maxs.y, maxs.z + dim.z),

		// own chunk region
		voxel::Region(mins, maxs)
	};
	// the assumption here is that we get a full heigt paging request, otherwise we
	// would have to loop over more regions.
	core_assert(pagerCtx.region.getLowerY() == 0);
	core_assert(pagerCtx.region.getUpperY() == voxel::MAX_HEIGHT);
	voxel::PagedVolumeWrapper wrapper(_volumeData, pagerCtx.chunk, pagerCtx.region);
	std::vector<const char*> treeTypes;

	for (size_t i = 0; i < regions.size(); ++i) {
		const voxel::Region& region = regions[i];
		treeTypes = _biomeManager.getTreeTypes(region);
		if (treeTypes.empty()) {
			Log::debug("No tree types given for region %s", region.toString().c_str());
			return;
		}
		const int border = 2;
		unsigned int seed = _seed + glm::abs(region.getCentreX() + region.getCentreZ());
		math::Random random(seed);

		random.shuffle(treeTypes.begin(), treeTypes.end());

		std::vector<glm::vec2> positions;
		_biomeManager.getTreePositions(region, positions, random, border);
		int treeTypeIndex = 0;
		const int regionY = region.getCentreY();
		const int treeTypeSize = (int)treeTypes.size();
		for (const glm::vec2& position : positions) {
			glm::ivec3 treePos(position.x, regionY, position.y);
			if (!_volumeData->hasChunk(treePos)) {
				continue;
			}
			treePos.y = findFloor(_volumeData, position.x, position.y);
			if (treePos.y <= voxel::MAX_WATER_HEIGHT) {
				continue;
			}
			const char *treeType = treeTypes[treeTypeIndex++];
			treeTypeIndex %= treeTypeSize;
			// TODO: if a tree is placed at the volume chunk sizes, and also overlaps the mesh extraction
			// chunk size, there are parts of the tree that are missing.
			// notice mapview with single position activated at -1, 0, 277 and 0, 0, 277
			// TODO: this hardcoded 10... no way
			const int treeIndex = 1 ; //random.random(1, 10);
			char filename[64];
			if (!core::string::formatBuf(filename, sizeof(filename), "models/trees/%s/%i.vox", treeType, treeIndex)) {
				Log::error("Failed to assemble tree path");
				continue;
			}
			const voxel::RawVolume* v = _volumeCache->loadVolume(filename);
			if (v == nullptr) {
				continue;
			}
			Log::debug("region %i: final treepos: %3i:%3i:%3i", (int)i, treePos.x, treePos.y, treePos.z);
			addVolumeToPosition(wrapper, v, treePos);
		}
	}
}

void WorldPager::addVolumeToPosition(voxel::PagedVolumeWrapper& target, const voxel::RawVolume* source, const glm::ivec3& pos) {
	const voxel::Region& region = source->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();
	const voxel::Region& targetRegion = target.region();
	for (int x = mins.x; x <= maxs.x; ++x) {
		const int nx = pos.x + x;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const int ny = pos.y + y;
			for (int z = mins.z; z <= maxs.z; ++z) {
				const int nz = pos.z + z;
				if (!targetRegion.containsPoint(nx, ny, nz)) {
					continue;
				}
				const voxel::Voxel& voxel = source->voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				target.setVoxel(nx, ny, nz, voxel);
			}
		}
	}
}

}
