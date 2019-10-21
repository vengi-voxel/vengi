/**
 * @file
 */

#include "WorldMgr.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "core/io/File.h"
#include "math/Random.h"
#include "core/Concurrency.h"
#include "voxel/polyvox/AStarPathfinder.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/Constants.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/Spiral.h"

namespace voxel {

WorldMgr::WorldMgr() :
		_threadPool(core::halfcpus(), "WorldMgr"), _random(_seed) {
}

WorldMgr::~WorldMgr() {
	shutdown();
}

glm::ivec3 WorldMgr::randomPos() const {
	int lowestX = -100;
	int lowestZ = -100;
	int highestX = 100;
	int highestZ = 100;
	for (const glm::ivec3& gridPos : _positionsExtracted) {
		lowestX = core_min(lowestX, gridPos.x);
		lowestZ = core_min(lowestZ, gridPos.z);
		highestX = core_min(highestX, gridPos.x);
		highestZ = core_min(highestZ, gridPos.z);
	}
	const int x = _random.random(lowestX, highestX);
	const int z = _random.random(lowestZ, highestZ);
	const int y = findFloor(x, z, isFloor);
	return glm::ivec3(x, y, z);
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool WorldMgr::scheduleMeshExtraction(const glm::ivec3& p) {
	if (_cancelThreads) {
		return false;
	}
	const glm::ivec3& pos = meshPos(p);
	auto i = _positionsExtracted.insert(pos);
	if (!i.second) {
		return false;
	}
	Log::trace("mesh extraction for %i:%i:%i (%i:%i:%i)",
			p.x, p.y, p.z, pos.x, pos.y, pos.z);
	_pendingExtraction.push(pos);
	return true;
}

void WorldMgr::setSeed(long seed) {
	Log::info("Seed is: %li", seed);
	_seed = seed;
	_random.setSeed(seed);
	_pager.setSeed(seed);
	_pager.setNoiseOffset(glm::vec2(_random.randomf(-10000.0f, 10000.0f), _random.randomf(-10000.0f, 10000.0f)));
}

PickResult WorldMgr::pickVoxel(const glm::vec3& origin, const glm::vec3& directionWithLength) {
	static constexpr voxel::Voxel air = voxel::createVoxel(voxel::VoxelType::Air, 0);
	return voxel::pickVoxel(_volumeData, origin, directionWithLength, air);
}

void WorldMgr::updateExtractionOrder(const glm::ivec3& sortPos, const math::Frustum& frustum) {
	static glm::ivec3 lastSortPos = glm::ivec3((std::numeric_limits<int>::max)());
	const glm::ivec3 d = lastSortPos - sortPos;
	const glm::ivec3::value_type allowedDelta = 10;
	if (d.x > allowedDelta || d.z > allowedDelta) {
		_pendingExtraction.sort([&] (const glm::ivec3& lhs, const glm::ivec3& rhs) {
			return glm::length2(glm::vec3(lhs - sortPos)) > glm::length2(glm::vec3(rhs - sortPos));
		});
	}
}

bool WorldMgr::allowReExtraction(const glm::ivec3& pos) {
	const glm::ivec3& gridPos = meshPos(pos);
	return _positionsExtracted.erase(gridPos) != 0;
}

bool WorldMgr::findPath(const glm::ivec3& start, const glm::ivec3& end,
		std::list<glm::ivec3>& listResult) {
	core_trace_scoped(FindPath);
	static auto f = [] (const voxel::PagedVolume* volData, const glm::ivec3& v3dPos) {
		const voxel::Voxel& voxel = volData->voxel(v3dPos);
		return isBlocked(voxel.getMaterial());
	};

	const AStarPathfinderParams<voxel::PagedVolume> params(_volumeData, start, end, &listResult, 1.0f, 10000,
			TwentySixConnected, std::bind(f, std::placeholders::_1, std::placeholders::_2));
	AStarPathfinder<voxel::PagedVolume> pf(params);
	// TODO: move into threadpool
	pf.execute();
	return true;
}

bool WorldMgr::init(const std::string& luaParameters, const std::string& luaBiomes, uint32_t volumeMemoryMegaBytes, uint16_t chunkSideLength) {
	_threadPool.init();
	if (!_biomeManager.init(luaBiomes)) {
		Log::error("Failed to init the biome mgr");
		return false;
	}
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);
	_volumeData = new PagedVolume(&_pager, volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);

	_pager.init(_volumeData, &_biomeManager, luaParameters);
	if (_clientData) {
		_pager.setCreateFlags(voxel::WORLDGEN_CLIENT);
	} else {
		_pager.setCreateFlags(voxel::WORLDGEN_SERVER);
	}

	for (size_t i = 0u; i < _threadPool.size(); ++i) {
		_threadPool.enqueue([this] () {extractScheduledMesh();});
	}

	return true;
}

void WorldMgr::extractScheduledMesh() {
	while (!_cancelThreads) {
		decltype(_pendingExtraction)::Key pos;
		if (!_pendingExtraction.waitAndPop(pos)) {
			break;
		}
		core_trace_scoped(MeshExtraction);
		const glm::ivec3& size = meshSize();
		const glm::ivec3 mins(pos);
		const glm::ivec3 maxs(glm::ivec3(pos) + size - 1);
		const Region region(mins, maxs);
		// these numbers are made up mostly by try-and-error - we need to revisit them from time to time to prevent extra mem allocs
		// they also heavily depend on the size of the mesh region we extract
		const int opaqueFactor = 16;
		const int opaqueVertices = region.getWidthInVoxels() * region.getDepthInVoxels() * opaqueFactor;
		const int waterVertices = region.getWidthInVoxels() * region.getDepthInVoxels();
		ChunkMeshes data(opaqueVertices, opaqueVertices, waterVertices, waterVertices);
		extractAllCubicMesh(_volumeData, region,
				&data.opaqueMesh, &data.waterMesh,
				IsQuadNeeded(), IsWaterQuadNeeded(),
				MAX_WATER_HEIGHT);
		if (!data.waterMesh.isEmpty() || !data.opaqueMesh.isEmpty()) {
			_extracted.push(std::move(data));
		}
	}
}

void WorldMgr::shutdown() {
	_cancelThreads = true;
	_pendingExtraction.clear();
	_pendingExtraction.abortWait();
	_extracted.clear();
	_extracted.abortWait();
	_threadPool.shutdown();
	_positionsExtracted.clear();
	_extracted.clear();
	_pager.shutdown();
	_biomeManager.shutdown();
	delete _volumeData;
	_volumeData = nullptr;
}

void WorldMgr::reset() {
	_cancelThreads = true;
}

bool WorldMgr::isReset() const {
	return _cancelThreads;
}

void WorldMgr::stats(int& meshes, int& extracted, int& pending) const {
	extracted = _positionsExtracted.size();
	pending = _pendingExtraction.size();
	meshes = _extracted.size();
}

bool WorldMgr::raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance, glm::ivec3& hit, Voxel& voxel) const {
	const bool result = raycast(start, direction, maxDistance, [&] (const PagedVolume::Sampler& sampler) {
		voxel = sampler.voxel();
		if (isBlocked(voxel.getMaterial())) {
			// store position and abort raycast
			hit = sampler.position();
			return false;
		}
		return true;
	});
	return result;
}

int WorldMgr::findWalkableFloor(const glm::vec3& position, float maxDistanceY) const {
	const voxel::VoxelType type = material(position.x, position.y, position.z);
	int y = NO_FLOOR_FOUND;
	if (voxel::isEnterable(type)) {
		raycast(position, glm::down, glm::min(maxDistanceY, position.y), [&] (const PagedVolume::Sampler& sampler) {
			voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (!voxel::isEnterable(mat)) {
				y = sampler.position().y + 1;
				return false;
			}
			return true;
		});
	} else {
		raycast(position, glm::up, glm::min(maxDistanceY, (float)MAX_HEIGHT - position.y), [&] (const PagedVolume::Sampler& sampler) {
			voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (voxel::isEnterable(mat)) {
				y = sampler.position().y;
				return false;
			}
			return true;
		});
	}
	return y;
}

int WorldMgr::chunkSize() const {
	return _volumeData->chunkSideLength();
}

glm::ivec3 WorldMgr::meshSize() const {
	const int s = _meshSize->intVal();
	return glm::ivec3(s, MAX_MESH_CHUNK_HEIGHT, s);
}

VoxelType WorldMgr::material(int x, int y, int z) const {
	const Voxel& voxel = _volumeData->voxel(x, y, z);
	return voxel.getMaterial();
}

}
