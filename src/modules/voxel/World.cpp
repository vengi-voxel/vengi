#include "World.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "io/File.h"
#include "LodCreator.h"
#include "core/Random.h"
#include "noise/SimplexNoise.h"
#include "generator/ShapeGenerator.h"
#include "generator/TreeGenerator.h"
#include <SDL.h>
#include "polyvox/AStarPathfinder.h"
#include "polyvox/CubicSurfaceExtractor.h"
#include "polyvox/RawVolume.h"
#include "polyvox/Voxel.h"

namespace voxel {

void World::Pager::erase(const Region& region, PagedVolume::Chunk* chunk) {
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = chunk;
	_worldPersister.erase(ctx, _world.seed());
}

void World::Pager::pageIn(const Region& region, PagedVolume::Chunk* chunk) {
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = chunk;
	if (!_worldPersister.load(ctx, _world.seed())) {
		_world.create(ctx);
	}
}

void World::Pager::pageOut(const Region& region, PagedVolume::Chunk* chunk) {
	TerrainContext ctx;
	ctx.region = region;
	ctx.chunk = chunk;
	_worldPersister.save(ctx, _world.seed());
}

// http://code.google.com/p/fortressoverseer/source/browse/Overseer/PolyVoxGenerator.cpp
World::World() :
		_pager(*this), _seed(0), _clientData(false), _threadPool(1), _rwLock("World"),
		_random(_seed), _noiseSeedOffsetX(0.0f), _noiseSeedOffsetZ(0.0f) {
	_chunkSize = core::Var::get(cfg::VoxelChunkSize, "64", core::CV_READONLY);
	_volumeData = new PagedVolume(&_pager, 256 * 1024 * 1024, _chunkSize->intVal());
	core_assert(_biomManager.addBiom(0, 100, createVoxel(Grass)));
	core_assert(_biomManager.addBiom(101, MAX_HEIGHT - 1, createVoxel(Grass)));
}

World::~World() {
	_cancelThreads = true;
	while (!_futures.empty()) {
		cleanupFutures();
	}
	locked([this] () {
		_meshesExtracted.clear();
		_meshQueue.clear();
	});
	delete _volumeData;
}

glm::ivec2 World::randomPosWithoutHeight(const Region& region, int border) {
	const int w = region.getWidthInVoxels();
	const int d = region.getDepthInVoxels();
	core_assert(border < w);
	core_assert(border < d);
	const int x = _random.random(border, w - border);
	const int z = _random.random(border, d - border);
	return glm::ivec2(x, z);
}

glm::ivec3 World::randomPos() const {
	// TODO: where the heck is the randomness
	const glm::ivec2 pos(0, 0);
	const int y = findFloor(pos.x, pos.y);
	return glm::ivec3(pos.x, y, pos.y);
}

struct IsVoxelTransparent {
	bool operator()(const Voxel& voxel) const {
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
	for (int nx = region.getLowerX() - 1; nx < region.getUpperX() + 1; ++nx) {
		for (int nz = region.getLowerZ() - 1; nz < region.getUpperZ() + 1; ++nz) {
			for (int ny = region.getLowerY(); ny < region.getUpperY() - 1; ++ny) {
				// if the voxel is air, we don't need to compute anything
				Voxel voxel = _volumeData->getVoxel(nx, ny, nz);
				if (voxel.getMaterial() == Air) {
					continue;
				}
				// if the voxel above us is not free - we don't calculate ao for this voxel
				if (_volumeData->getVoxel(nx, ny + 1, nz).getMaterial() != Air) {
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
		core_trace_scoped("MeshExtraction");
		const Region& region = getRegion(pos);
		DecodedMeshData data;
		{
			locked([&] () {
				// calculate ao
				calculateAO(region);
				const bool mergeQuads = true;
				data.mesh[0] = decodeMesh(extractCubicMesh(_volumeData, region, IsQuadNeeded(), mergeQuads));

				const uint32_t downScaleFactor = 2;
				for (data.numLods = 1; data.numLods < 2; ++data.numLods) {
					Region srcRegion = region;
					srcRegion.grow(downScaleFactor);

					const glm::ivec3& lowerCorner = srcRegion.getLowerCorner();
					glm::ivec3 upperCorner = srcRegion.getUpperCorner();

					upperCorner = upperCorner - lowerCorner;
					upperCorner = upperCorner / 2;
					upperCorner = upperCorner + lowerCorner;

					Region targetRegion(lowerCorner, upperCorner);

					RawVolume rawVolume(targetRegion);
					rescaleCubicVolume(_volumeData, srcRegion, &rawVolume, rawVolume.getEnclosingRegion());
					targetRegion.shrink(1);
					data.mesh[data.numLods] = decodeMesh(extractCubicMesh(&rawVolume, targetRegion, IsQuadNeeded(), mergeQuads));
				}
			});
		}

		data.translation = pos;
		core::ScopedWriteLock lock(_rwLock);
		_meshQueue.push_back(std::move(data));
	}));
	return true;
}

Region World::getRegion(const glm::ivec3& pos) const {
	const int size = _chunkSize->intVal();
	int deltaX = size - 1;
	int deltaY = size - 1;
	int deltaZ = size - 1;
	const glm::ivec3 mins(pos.x, pos.y, pos.z);
	const glm::ivec3 maxs(pos.x + deltaX, pos.y + deltaY, pos.z + deltaZ);
	const Region region(mins, maxs);
	return region;
}

void World::placeTree(const TreeContext& ctx) {
	const glm::ivec3 pos(ctx.pos.x, findFloor(ctx.pos.x, ctx.pos.y), ctx.pos.y);
	const Region& region = getRegion(getGridPos(pos));
	TerrainContext tctx;
	tctx.chunk = nullptr;
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
	static auto f = [] (const voxel::PagedVolume* volData, const glm::ivec3& v3dPos) {
		const voxel::Voxel& voxel = volData->getVoxel(v3dPos);
		return voxel.getMaterial() != Air;
	};

	locked([&] () {
		const AStarPathfinderParams<voxel::PagedVolume> params(_volumeData, start, end, &listResult, 1.0f, 10000,
				TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
		AStarPathfinder<voxel::PagedVolume> pf(params);
		// TODO: move into threadpool
		pf.execute();
	});
	return true;
}

void World::destroy() {
	reset();
}

void World::reset() {
	_cancelThreads = true;
}

bool World::isValidChunkPosition(TerrainContext& ctx, const glm::ivec3& pos) const {
	if (ctx.chunk == nullptr) {
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

void World::createClouds(TerrainContext& ctx) {
	const int amount = 4;
	static const Voxel voxel = createVoxel(Cloud);
	for (int i = 0; i < amount; ++i) {
		const int height = 10;
		const glm::ivec2& pos = randomPosWithoutHeight(ctx.region, 20);
		glm::ivec3 chunkCloudCenterPos(pos.x, ctx.region.getHeightInVoxels() - height, pos.y);
		ShapeGenerator::createEllipse(ctx, chunkCloudCenterPos, 10, height, 10, voxel);
		chunkCloudCenterPos.x -= 5;
		chunkCloudCenterPos.y -= 5 + i;
		ShapeGenerator::createEllipse(ctx, chunkCloudCenterPos, 20, height, 20, voxel);
	}
}

void World::createUnderground(TerrainContext& ctx) {
	glm::ivec3 startPos(1, 1, 1);
	const Voxel voxel = createVoxel(Grass);
	ShapeGenerator::createPlane(ctx, startPos, 10, 10, voxel);
}

void World::create(TerrainContext& ctx) {
	const Region& region = ctx.region;
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int height = region.getHeightInVoxels();
	const int lowerY = region.getLowerY();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();
	// TODO: kill me
	const core::VarPtr& plainTerrain = core::Var::get("voxel-plainterrain", "false");
	const bool plainTerrainBool = plainTerrain->boolVal();

	// TODO: the 2d noise doesn't neep the same resolution - we can optimize this a lot
	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const glm::vec2 noisePos2d = glm::vec2(_noiseSeedOffsetX + lowerX + x, _noiseSeedOffsetZ + lowerZ + z);
			const float landscapeNoise = noise::Simplex::Noise2D(noisePos2d, _ctx.landscapeNoiseOctaves,
					_ctx.landscapeNoisePersistence, _ctx.landscapeNoiseFrequency, _ctx.landscapeNoiseAmplitude);
			const float noiseNormalized = noise::norm(landscapeNoise);
			const float mountainNoise = noise::Simplex::Noise2D(noisePos2d, _ctx.mountainNoiseOctaves,
					_ctx.mountainNoisePersistence, _ctx.mountainNoiseFrequency, _ctx.mountainNoiseAmplitude);
			const float mountainNoiseNormalized = noise::norm(mountainNoise);
			const float mountainMultiplier = mountainNoiseNormalized * (mountainNoiseNormalized + 0.5f);
			const float n = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			const int ni = n * (MAX_TERRAIN_HEIGHT - 1);
			int y = 0;
			int start = lowerY;
			if (start == y) {
				++start;
				const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, 0, lowerZ + z);
				ctx.chunk->setVoxel(x, 0, z, voxel);
			}
			if (plainTerrainBool) {
				for (int h = lowerY; h < ni; ++h) {
					const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
					ctx.chunk->setVoxel(x, y, z, voxel);
				}
			} else {
				for (int h = lowerY; h < ni; ++h) {
					const glm::vec3 noisePos3d = glm::vec3(noisePos2d.x, h, noisePos2d.y);
					const float noiseVal = noise::norm(noise::Simplex::Noise3D(noisePos3d, _ctx.caveNoiseOctaves,
							_ctx.caveNoisePersistence, _ctx.caveNoiseFrequency, _ctx.caveNoiseAmplitude));
					const float finalDensity = noiseNormalized + noise::norm(noiseVal);
					if (finalDensity > _ctx.caveDensityThreshold) {
						const Voxel& voxel = _biomManager.getVoxelType(lowerX + x, h, lowerZ + z);
						ctx.chunk->setVoxel(x, y, z, voxel);
					}
					if (++y >= height) {
						break;
					}
				}
			}
		}
	}
	const glm::vec3 worldPos(lowerX, lowerY, lowerZ);
	if (_clientData && _biomManager.hasClouds(worldPos)) {
		createClouds(ctx);
	}
	if (_biomManager.hasTrees(worldPos)) {
		TreeGenerator::createTrees(ctx, _random);
	}

	for (const TerrainContext::NonChunkVoxel& voxelData : ctx.nonChunkVoxels) {
		const glm::ivec3& pos = voxelData.pos;
		_volumeData->setVoxel(pos, voxelData.voxel);
		if (region.containsPoint(pos.x, pos.y, pos.z)) {
			continue;
		}
		if (!allowReExtraction(pos)) {
			continue;
		}
		scheduleMeshExtraction(pos);
	}
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
	cleanupFutures();
	if (_cancelThreads) {
		if (!_futures.empty()) {
			return;
		}
		locked([this] () {
			_volumeData->flushAll();
			_ctx = WorldContext();
			_meshesExtracted.clear();
			_meshQueue.clear();
			Log::info("reset the world");
		});
		_cancelThreads = false;
	}
}

bool World::isReset() const {
	return _cancelThreads;
}

}
