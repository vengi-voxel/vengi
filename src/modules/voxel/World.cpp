/**
 * @file
 */

#include "World.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "core/Random.h"
#include "core/Concurrency.h"
#include "voxel/generator/WorldGenerator.h"
#include "voxel/polyvox/AStarPathfinder.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/Constants.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/Spiral.h"

namespace voxel {

#define PERSIST 1

void World::WorldPager::erase(PagedVolume::PagerContext& pctx) {
#if PERSIST
	PagedVolumeWrapper ctx(_world._volumeData, pctx.chunk, pctx.region);
	_worldPersister.erase(ctx, _world.seed());
#endif
}

bool World::WorldPager::pageIn(PagedVolume::PagerContext& pctx) {
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	PagedVolumeWrapper ctx(_world._volumeData, pctx.chunk, pctx.region);
#if PERSIST
	if (_world._persist && _worldPersister.load(ctx, _world.seed())) {
		return false;
	}
#endif
	_world.create(ctx);
	return true;
}

void World::WorldPager::pageOut(PagedVolume::PagerContext& pctx) {
#if PERSIST
	if (!_world._persist) {
		return;
	}
	PagedVolumeWrapper ctx(_world._volumeData, pctx.chunk, pctx.region);
	_worldPersister.save(ctx, _world.seed());
#endif
}

World::World() :
		_pager(*this), _threadPool(core::halfcpus(), "World"), _random(_seed) {
}

World::~World() {
	_cancelThreads = true;
	while (!_futures.empty()) {
		cleanupFutures();
	}
	_meshesExtracted.clear();
	_meshQueue.clear();
	delete _volumeData;
}

glm::ivec3 World::randomPos() const {
	int lowestX = -100;
	int lowestZ = -100;
	int highestX = 100;
	int highestZ = 100;
	for (const glm::ivec3& gridPos : _meshesExtracted) {
		lowestX = std::min(lowestX, gridPos.x);
		lowestZ = std::min(lowestZ, gridPos.z);
		highestX = std::min(highestX, gridPos.x);
		highestZ = std::min(highestZ, gridPos.z);
	}
	const int x = _random.random(lowestX, highestX);
	const int z = _random.random(lowestZ, highestZ);
	const int y = findFloor(x, z, isFloor);
	return glm::ivec3(x, y, z);
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool World::scheduleMeshExtraction(const glm::ivec3& p) {
	if (_cancelThreads) {
		return false;
	}
	const glm::ivec3& pos = getMeshPos(p);
	auto i = _meshesExtracted.find(pos);
	if (i != _meshesExtracted.end()) {
		Log::trace("mesh is already extracted for %i:%i:%i (%i:%i:%i - %i:%i:%i)", p.x, p.y, p.z, i->x, i->y, i->z, pos.x, pos.y, pos.z);
		return false;
	}
	Log::trace("mesh extraction for %i:%i:%i (%i:%i:%i)", p.x, p.y, p.z, pos.x, pos.y, pos.z);
	_meshesExtracted.insert(pos);

	_futures.push_back(_threadPool.enqueue([=] () {
		if (_cancelThreads) {
			return;
		}
		core_trace_scoped(MeshExtraction);
		const Region &region = getMeshRegion(pos);

		const int extends = 3;
		glm::ivec3 mins = region.getLowerCorner() - extends;
		mins.y = 0;
		glm::ivec3 maxs = region.getUpperCorner() + extends;
		maxs.y = MAX_HEIGHT - 1;
		const Region prefetchRegion(mins, maxs);
		_volumeData->prefetch(prefetchRegion);

		// these number are made up mostly by try-and-error - we need to revisit them from time to time to prevent extra mem allocs
		// they also heavily depend on the size of the mesh region we extract
		const int opaqueFactor = 16;
		const int waterFactor = 16;
		const int opaqueVertices = region.getWidthInVoxels() * region.getDepthInVoxels() * opaqueFactor;
		const int waterVertices = region.getWidthInVoxels() * region.getDepthInVoxels() * waterFactor;
		ChunkMeshes data(opaqueVertices, opaqueVertices, waterVertices, waterVertices);
		extractCubicMesh(_volumeData, region, &data.opaqueMesh, IsQuadNeeded());
		extractCubicMesh(_volumeData, region, &data.waterMesh, IsWaterQuadNeeded());
#if 0
		Log::info("opaque mesh size: %i", (int)data.opaqueMesh.size());
		Log::info("water mesh size: %i", (int)data.waterMesh.size());
#endif
		{
			LockGuard lock(_rwLock);
			_meshQueue.push_back(std::move(data));
		}
		_meshQueueEmpty = false;
	}));
	return true;
}

Region World::getRegion(const glm::ivec3& pos, int size) const {
	int deltaX = size - 1;
	int deltaZ = size - 1;
	const glm::ivec3 mins(pos.x, 0, pos.z);
	const glm::ivec3 maxs(pos.x + deltaX, MAX_HEIGHT - 1, pos.z + deltaZ);
	const Region region(mins, maxs);
	return region;
}

void World::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	_volumeData->setVoxel(pos, voxel);
}

bool World::allowReExtraction(const glm::ivec3& pos) {
	const glm::ivec3& gridPos = getMeshPos(pos);
	return _meshesExtracted.erase(gridPos) != 0;
}

bool World::findPath(const glm::ivec3& start, const glm::ivec3& end,
		std::list<glm::ivec3>& listResult) {
	core_trace_scoped(FindPath);
	static auto f = [] (const voxel::PagedVolume* volData, const glm::ivec3& v3dPos) {
		const voxel::Voxel& voxel = volData->getVoxel(v3dPos);
		return isBlocked(voxel.getMaterial());
	};

	const AStarPathfinderParams<voxel::PagedVolume> params(_volumeData, start, end, &listResult, 1.0f, 10000,
			TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
	AStarPathfinder<voxel::PagedVolume> pf(params);
	// TODO: move into threadpool
	pf.execute();
	return true;
}

bool World::init(const io::FilePtr& luaFile) {
	if (!_biomeManager.init()) {
		return false;
	}
	if (!_ctx.load(luaFile)) {
		return false;
	}
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);
	_volumeData = new PagedVolume(&_pager, 512 * 1024 * 1024, 256);
	// TODO: move into lua
	_biomeManager.addBiom(0, MAX_WATER_HEIGHT + 4, 0.5f, 0.5f, VoxelType::Sand);
	_biomeManager.addBiom(0, MAX_TERRAIN_HEIGHT - 1, 0.1f, 0.9f, VoxelType::Sand);
	_biomeManager.addBiom(MAX_WATER_HEIGHT + 3, MAX_WATER_HEIGHT + 10, 1.0f, 0.7f, VoxelType::Dirt);
	_biomeManager.addBiom(MAX_WATER_HEIGHT + 3, MAX_TERRAIN_HEIGHT + 1, 0.5f, 0.5f, VoxelType::Grass);
	_biomeManager.addBiom(MAX_TERRAIN_HEIGHT - 20, MAX_TERRAIN_HEIGHT + 1, 0.4f, 0.5f, VoxelType::Rock);
	_biomeManager.addBiom(0, MAX_TERRAIN_HEIGHT - 1, 0.4f, 0.5f, VoxelType::Rock, true);
	return true;
}

void World::shutdown() {
	reset();
}

void World::reset() {
	_cancelThreads = true;
}

void World::create(PagedVolumeWrapper& ctx) {
	core_trace_scoped(CreateWorld);
	const int flags = _clientData ? world::WORLDGEN_CLIENT : world::WORLDGEN_SERVER;
	world::createWorld(_ctx, ctx, _biomeManager, _seed, flags, _noiseSeedOffsetX, _noiseSeedOffsetZ);
}

void World::cleanupFutures() {
	for (auto i = _futures.begin(); i != _futures.end();) {
		auto& future = *i;
		if (std::future_status::ready == future.wait_for(std::chrono::milliseconds::zero())) {
			i = _futures.erase(i);
			continue;
		}
		break;
	}
}

void World::prefetch(const glm::vec3& pos) {
	_futures.push_back(_threadPool.enqueue([=] () {
		if (_cancelThreads) {
			return;
		}
		_volumeData->prefetch(getRegion(pos, 1024));
	}));
}

void World::onFrame(long dt) {
	core_trace_scoped(WorldOnFrame);
	cleanupFutures();
	if (_cancelThreads) {
		if (!_futures.empty()) {
			return;
		}
		_volumeData->flushAll();
		_ctx = WorldContext();
		_meshesExtracted.clear();
		_meshQueue.clear();
		_meshQueueEmpty = true;
		Log::info("reset the world");
		_cancelThreads = false;
	}
}

bool World::isReset() const {
	return _cancelThreads;
}

void World::stats(int& meshes, int& extracted, int& pending) const {
	extracted = _meshesExtracted.size();
	pending = _futures.size();
	if (_meshQueueEmpty) {
		meshes = 0;
		return;
	}
	LockGuard lock(_rwLock);
	meshes = _meshQueue.size();
}

bool World::raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance, glm::ivec3& hit, Voxel& voxel) const {
	const bool result = raycast(start, direction, maxDistance, [&] (const PagedVolume::Sampler& sampler) {
		voxel = sampler.getVoxel();
		if (isBlocked(voxel.getMaterial())) {
			// store position and abort raycast
			hit = sampler.getPosition();
			return false;
		}
		return true;
	});
	return result;
}

}
