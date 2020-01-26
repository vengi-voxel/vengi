/**
 * @file
 */
#include "WorldPager.h"
#include "math/Random.h"
#include "core/ArrayLength.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxelutil/Raycast.h"
#include "noise/Simplex.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/collection/Array.h"

namespace voxelworld {

WorldPager::WorldPager(const voxelformat::VolumeCachePtr& volumeCache, const ChunkPersisterPtr& chunkPersister) :
		_volumeCache(volumeCache), _chunkPersister(chunkPersister) {
}

void WorldPager::erase(const voxel::Region& region) {
	_chunkPersister->erase(region, _seed);
}

bool WorldPager::pageIn(voxel::PagedVolume::PagerContext& pctx) {
	core_assert(_volumeData != nullptr);
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	if (_chunkPersister->load(pctx.chunk.get(), _seed)) {
		return false;
	}
	voxel::PagedVolumeWrapper wrapper(_volumeData, pctx.chunk, pctx.region);
	//if (pctx.region.getLowerX() == 0 && pctx.region.getLowerZ() == 0) {
	core_trace_scoped(CreateWorld);
	math::Random random(_seed);
	createWorld(wrapper);
	placeTrees(pctx);
	_chunkPersister->save(pctx.chunk.get(), _seed);
	//}
	return true;
}

void WorldPager::pageOut(voxel::PagedVolume::Chunk* chunk) {
	// currently chunks are not modifiable and are saved directly after creating the chunk
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
	if (!_volumeCache.init()) {
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
	_volumeCache.shutdown();
	_volumeData = nullptr;
	_biomeManager.shutdown();
	_worldCtx = WorldContext();
}

// use a 2d noise to switch between different noises - to generate steep mountains
void WorldPager::createWorld(voxel::PagedVolumeWrapper& volume) const {
	core_trace_scoped(WorldGeneration);
	const voxel::Region& region = volume.region();
	Log::debug("Create new chunk at %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int minsY = region.getLowerY();
	const int lowerZ = region.getLowerZ();
	core_assert(region.getLowerY() >= 0);
	voxel::Voxel voxels[voxel::MAX_TERRAIN_HEIGHT];

	// TODO: store voxel data in local buffer and transfer in one step into the volume to reduce locking
	const int size = 2;
	core_assert(depth % size == 0);
	core_assert(width % size == 0);
	for (int z = lowerZ; z < lowerZ + depth; z += size) {
		for (int x = lowerX; x < lowerX + width; x += size) {
			const int ni = fillVoxels(x, minsY, z, voxels);
			volume.setVoxels(x, minsY, z, size, size, voxels, ni);
			memset(voxels, 0, ni * sizeof(voxel::Voxel));
		}
	}
}

float WorldPager::getNoiseValue(float x, float z) const {
	const glm::vec2 noisePos2d(_noiseSeedOffset.x + x, _noiseSeedOffset.y + z);
	// TODO: move the noise settings into the biome
	const float landscapeNoise = noise::fBm(noisePos2d * _worldCtx.landscapeNoiseFrequency, _worldCtx.landscapeNoiseOctaves,
			_worldCtx.landscapeNoiseLacunarity, _worldCtx.landscapeNoiseGain);
	const float noiseNormalized = noise::norm(landscapeNoise);
	const float mountainNoise = noise::fBm(noisePos2d * _worldCtx.mountainNoiseFrequency, _worldCtx.mountainNoiseOctaves,
			_worldCtx.mountainNoiseLacunarity, _worldCtx.mountainNoiseGain);
	const float mountainNoiseNormalized = noise::norm(mountainNoise);
	const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
	const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
	return n;
}

float WorldPager::getDensity(float x, float y, float z, float n) const {
	const glm::vec3 noisePos3d(_noiseSeedOffset.x + x, y, _noiseSeedOffset.y + z);
	// TODO: move the noise settings into the biome
	const float noiseVal = noise::norm(
			noise::fBm(noisePos3d * _worldCtx.caveNoiseFrequency, _worldCtx.caveNoiseOctaves, _worldCtx.caveNoiseLacunarity, _worldCtx.caveNoiseGain));
	const float finalDensity = n + noiseVal;
	return finalDensity;
}

int WorldPager::terrainHeight(int x, int y, int z) const {
	const float n = getNoiseValue(x, z);
	return terrainHeight(x, y, z, n);
}

int WorldPager::terrainHeight(int x, int minsY, int z, float n) const {
	const int maxHeight = voxel::MAX_TERRAIN_HEIGHT - 1;
	int centerHeight;
	// the center of a city should make the terrain more even
	const float cityMultiplier = _biomeManager.getCityMultiplier(glm::ivec2(x, z), &centerHeight);
	int ni;
	if (cityMultiplier < 1.0f) {
		const float revn = (1.0f - cityMultiplier);
		ni = revn * centerHeight + (cityMultiplier * n * maxHeight);
	} else {
		ni = n * maxHeight;
	}
	for (int y = ni - 1; y >= minsY + 1; --y) {
		const float density = getDensity(x, y, z, n);
		if (density > _worldCtx.caveDensityThreshold) {
			break;
		}
		--ni;
	}
	return ni;
}

int WorldPager::fillVoxels(int x, int minsY, int z, voxel::Voxel* voxels) const {
	const float n = getNoiseValue(x, z);
	const int ni = terrainHeight(x, minsY, z, n);
	if (ni < minsY) {
		return 0;
	}

	const voxel::Voxel& water = createColorVoxel(voxel::VoxelType::Water, _seed);
	const voxel::Voxel& dirt = createColorVoxel(voxel::VoxelType::Dirt, _seed);
	static constexpr voxel::Voxel air;

	voxels[0] = dirt;
	glm::ivec3 pos(x, 0, z);
	for (int y = ni - 1; y >= minsY + 1; --y) {
		const float density = getDensity(x, y, z, n);
		if (density > _worldCtx.caveDensityThreshold) {
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
	for (int i = minsY; i < voxel::MAX_WATER_HEIGHT; ++i) {
		if (voxels[i] == air) {
			voxels[i] = water;
		}
	}
	return core_max(ni - minsY, voxel::MAX_WATER_HEIGHT - minsY);
}

void WorldPager::placeTrees(voxel::PagedVolume::PagerContext& pagerCtx) {
	// expand region to all surrounding regions by half of the region size.
	// we do this to be able to limit the generation on the current chunk. Otherwise
	// we would endlessly generate new chunks just because the trees overlap to
	// another chunk.
	const glm::ivec3& mins = pagerCtx.region.getLowerCorner();
	const glm::ivec3& maxs = pagerCtx.region.getUpperCorner();
	const glm::ivec3& dim = pagerCtx.region.getDimensionsInVoxels();
	const voxel::Region regions[] = {
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
	voxel::PagedVolumeWrapper chunkWrapper(_volumeData, pagerCtx.chunk, pagerCtx.region);
	std::vector<const char*> treeTypes;

	const size_t regionsSize = lengthof(regions);

	for (size_t i = 0; i < regionsSize; ++i) {
		const voxel::Region& region = regions[i];
		treeTypes = _biomeManager.getTreeTypes(region);
		if (treeTypes.empty()) {
			Log::debug("No tree types given for region %s", region.toString().c_str());
			return;
		}
		std::vector<glm::vec2> positions;
		{
			math::Random random(_seed);
			random.shuffle(treeTypes.begin(), treeTypes.end());
			_biomeManager.getTreePositions(region, positions, random, 0);
		}
		int treeTypeIndex = 0;
		const int treeTypeSize = (int)treeTypes.size();
		const math::Axis axes[] = {math::Axis::None, math::Axis::Y, math::Axis::Y, math::Axis::None, math::Axis::Y};
		constexpr size_t axesSize = lengthof(axes);
		int positionIndex = 0;
		for (const glm::vec2& position : positions) {
			++positionIndex;
			glm::ivec3 treePos(position.x, 0, position.y);
			treePos.y = terrainHeight(position.x, pagerCtx.region.getLowerY(), position.y);
			if (treePos.y <= voxel::MAX_WATER_HEIGHT) {
				continue;
			}
			const char *treeType = treeTypes[treeTypeIndex++];
			treeTypeIndex %= treeTypeSize;
			const voxel::RawVolume* v = _volumeCache.loadTree(treePos, treeType);
			if (v == nullptr) {
				continue;
			}
			const voxelutil::RawVolumeRotateWrapper rotateWrapper(v, axes[positionIndex % axesSize]);
			addVolumeToPosition(chunkWrapper, rotateWrapper, treePos);
		}
	}
}

void WorldPager::addVolumeToPosition(voxel::PagedVolumeWrapper& target, const voxelutil::RawVolumeRotateWrapper& source, const glm::ivec3& pos) {
	const voxel::Region& region = source.region();
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
				const voxel::Voxel& voxel = source.voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				target.setVoxel(nx, ny, nz, voxel);
			}
		}
	}
}

}
