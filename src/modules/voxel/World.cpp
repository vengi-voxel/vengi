#include "World.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "LodCreator.h"
#include "core/Random.h"
#include "generator/ShapeGenerator.h"
#include "generator/TreeGenerator.h"
#include "generator/CloudGenerator.h"
#include "generator/WorldGenerator.h"
#include <SDL.h>
#include "polyvox/AStarPathfinder.h"
#include "polyvox/CubicSurfaceExtractor.h"
#include "polyvox/RawVolume.h"
#include "polyvox/Voxel.h"
#include "Voxel.h"

namespace voxel {

#define PERSIST 1

void World::Pager::erase(const Region& region, PagedVolume::Chunk* chunk) {
#if PERSIST
	TerrainContext ctx;
	ctx.region = region;
	ctx.volume = _world._volumeData;
	_worldPersister.erase(ctx, chunk, _world.seed());
#endif
}

void World::Pager::pageIn(const Region& region, PagedVolume::Chunk* chunk) {
#if PERSIST
	TerrainContext ctx;
	ctx.region = region;
	ctx.volume = _world._volumeData;
	_worldPersister.load(ctx, chunk, _world.seed());
#endif
}

void World::Pager::pageOut(const Region& region, PagedVolume::Chunk* chunk) {
#if PERSIST
	TerrainContext ctx;
	ctx.region = region;
	ctx.volume = _world._volumeData;
	_worldPersister.save(ctx, chunk, _world.seed());
#endif
}

// http://code.google.com/p/fortressoverseer/source/browse/Overseer/PolyVoxGenerator.cpp
World::World() :
		_pager(*this), _seed(0), _clientData(false), _threadPool(4, "World"), _rwLock("World"),
		_random(_seed), _noiseSeedOffsetX(0.0f), _noiseSeedOffsetZ(0.0f) {
	_chunkSize = core::Var::get(cfg::VoxelChunkSize, "64", core::CV_READONLY);
	_volumeData = new PagedVolume(&_pager, 256 * 1024 * 1024, 64);
	core_assert(_biomManager.addBiom(0, 100, createVoxel(Grass)));
	core_assert(_biomManager.addBiom(101, MAX_HEIGHT - 1, createVoxel(Grass)));
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
	int lowestX = 0;
	int lowestZ = 0;
	int highestX = 0;
	int highestZ = 0;
	for (const glm::ivec3& gridPos : _meshesExtracted) {
		lowestX = std::min(lowestX, gridPos.x);
		lowestZ = std::min(lowestZ, gridPos.z);
		highestX = std::min(highestX, gridPos.x);
		highestZ = std::min(highestZ, gridPos.z);
	}
	const int x = _random.random(lowestX, highestX);
	const int z = _random.random(lowestZ, highestZ);
	const int y = findFloor(x, z);
	return glm::ivec3(x, y, z);
}

struct IsVoxelTransparent {
	inline bool operator()(const Voxel& voxel) const {
		return voxel.getMaterial() == Air;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse) const {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

void World::calculateAO(const Region& region) {
	core_trace_scoped(CalculateAO);
	const IsVoxelTransparent trans;
	for (int nx = region.getLowerX() - 1; nx < region.getUpperX() + 1; ++nx) {
		for (int nz = region.getLowerZ() - 1; nz < region.getUpperZ() + 1; ++nz) {
			for (int ny = region.getLowerY(); ny < region.getUpperY() - 1; ++ny) {
				// if the voxel is air, we don't need to compute anything
				const Voxel& voxel = _volumeData->getVoxel(nx, ny, nz);
				if (trans(voxel)) {
					continue;
				}
				// if the voxel above us is not free - we don't calculate ao for this voxel
				if (!trans(_volumeData->getVoxel(nx, ny + 1, nz).getMaterial())) {
					continue;
				}
				static const struct offsets {
					int x;
					int z;
				} of[] = {
					{ 1,  0}, { 1, -1}, {0, -1}, {-1, -1},
					{-1,  0}, {-1,  1}, {0,  1}, { 1,  1}
				};
				// reduce ao value to make a voxel face darker
				uint8_t ao = 255;
				for (int i = 0; i < (int)SDL_arraysize(of); ++i) {
					const int offX = of[i].x;
					const int offZ = of[i].z;
					const Voxel& voxel = _volumeData->getVoxel(nx + offX, ny + 1, nz + offZ);
					if (voxel.getMaterial() != Air) {
						ao -= 25;
					}
				}
				//voxel.setDensity(ao);
				_volumeData->setVoxel(nx, ny, nz, voxel);
			}
		}
	}
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool World::scheduleMeshExtraction(const glm::ivec3& p) {
	if (_cancelThreads)
		return false;
	const glm::ivec3& pos = getGridPos(p);
	auto i = _meshesExtracted.find(pos);
	if (i != _meshesExtracted.end()) {
		Log::trace("mesh is already extracted for %i:%i:%i (%i:%i:%i - %i:%i:%i)", p.x, p.y, p.z, i->x, i->y, i->z, pos.x, pos.y, pos.z);
		return false;
	}
	Log::trace("mesh extraction for %i:%i:%i (%i:%i:%i)", p.x, p.y, p.z, pos.x, pos.y, pos.z);
	_meshesExtracted.insert(pos);

	_futures.push_back(_threadPool.enqueue([=] () {
		if (_cancelThreads)
			return;
		core_trace_scoped(MeshExtraction);
		const Region& region = getRegion(pos);
		DecodedMeshData data;

		calculateAO(region);
		TerrainContext ctx;
		ctx.region = region;
		ctx.volume = _volumeData;
		create(ctx);
		const bool mergeQuads = true;
		data.mesh[0] = decodeMesh(extractCubicMesh(_volumeData, region, IsQuadNeeded(), mergeQuads));

		data.translation = pos;
		core::ScopedWriteLock lock(_rwLock);
		_meshQueue.push_back(std::move(data));
	}));
	return true;
}

Region World::getRegion(const glm::ivec3& pos) const {
	const int size = _chunkSize->intVal();
	int deltaX = size - 1;
	int deltaZ = size - 1;
	const glm::ivec3 mins(pos.x, 0, pos.z);
	const glm::ivec3 maxs(pos.x + deltaX, MAX_HEIGHT - 1, pos.z + deltaZ);
	const Region region(mins, maxs);
	return region;
}

void World::placeTree(const TreeContext& ctx) {
	core_trace_scoped(PlaceTree);
	const glm::ivec3 pos(ctx.pos.x, findFloor(ctx.pos.x, ctx.pos.y), ctx.pos.y);
	const Region& region = getRegion(getGridPos(pos));
	TerrainContext tctx;
	tctx.volume = nullptr;
	tctx.region = region;
	TreeGenerator::addTree(tctx, pos, ctx.type, ctx.trunkHeight, ctx.trunkWidth, ctx.width, ctx.depth, ctx.height, _random);
}

int World::findFloor(int x, int z) const {
	for (int i = MAX_HEIGHT; i >= 0; i--) {
		const int material = getMaterial(x, i, z);
		if (isFloor(material)) {
			return i + 1;
		}
	}
	return -1;
}

bool World::allowReExtraction(const glm::ivec3& pos) {
	const glm::ivec3& gridPos = getGridPos(pos);
	return _meshesExtracted.erase(gridPos) != 0;
}

bool World::findPath(const glm::ivec3& start, const glm::ivec3& end,
		std::list<glm::ivec3>& listResult) {
	core_trace_scoped(FindPath);
	static auto f = [] (const voxel::PagedVolume* volData, const glm::ivec3& v3dPos) {
		const voxel::Voxel& voxel = volData->getVoxel(v3dPos);
		return voxel.getMaterial() != Air;
	};

	const AStarPathfinderParams<voxel::PagedVolume> params(_volumeData, start, end, &listResult, 1.0f, 10000,
			TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
	AStarPathfinder<voxel::PagedVolume> pf(params);
	// TODO: move into threadpool
	pf.execute();
	return true;
}

void World::destroy() {
	reset();
}

void World::reset() {
	_cancelThreads = true;
}

bool World::isValidChunkPosition(TerrainContext& ctx, const glm::ivec3& pos) const {
	if (ctx.volume == nullptr) {
		return false;
	}

	if (pos.x < 0 || pos.x >= ctx.region.getWidthInVoxels())
		return false;
	if (pos.y < 0 || pos.y >= ctx.region.getHeightInVoxels())
		return false;
	if (pos.z < 0 || pos.z >= ctx.region.getDepthInVoxels())
		return false;
	return true;
}

void World::createUnderground(TerrainContext& ctx) {
	const glm::ivec3 startPos(1, 1, 1);
	const Voxel& voxel = createVoxel(Grass);
	ShapeGenerator::createPlane(ctx, startPos, 10, 10, voxel);
}

void World::create(TerrainContext& ctx) {
	core_trace_scoped(CreateWorld);
	const int flags = _clientData ? WORLDGEN_CLIENT : WORLDGEN_SERVER;
	WorldGenerator::createWorld(_ctx, ctx, _biomManager, _random, flags, _noiseSeedOffsetX, _noiseSeedOffsetZ);
}

void World::cleanupFutures() {
	for (auto i = _futures.begin(); i != _futures.end();) {
		auto& future = *i;
		if (std::future_status::ready == future.wait_for(std::chrono::milliseconds(0))) {
			i = _futures.erase(i);
			continue;
		}
		break;
	}
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
		Log::info("reset the world");
		_cancelThreads = false;
	}
}

bool World::isReset() const {
	return _cancelThreads;
}

}
