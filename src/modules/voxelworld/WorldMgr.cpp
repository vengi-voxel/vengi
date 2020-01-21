/**
 * @file
 */

#include "WorldMgr.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "core/io/File.h"
#include "math/Random.h"
#include "core/Concurrency.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/PagedVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxel/Constants.h"
#include "voxel/IsQuadNeeded.h"

namespace voxelworld {

WorldMgr::WorldMgr(const voxel::PagedVolume::PagerPtr& pager) :
		_pager(pager), _threadPool(core::halfcpus(), "WorldMgr"), _random(_seed) {
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
	const int y = findFloor(x, z, voxel::isFloor);
	return glm::ivec3(x, y, z);
}

void WorldMgr::reset() {
	_extracted.clear();
	_positionsExtracted.clear();
	_pendingExtraction.clear();
	_volumeData->flushAll();
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

void WorldMgr::setSeed(unsigned int seed) {
	Log::info("Seed is: %u", seed);
	_seed = seed;
	_random.setSeed(seed);
}

void WorldMgr::updateExtractionOrder(const glm::ivec3& sortPos) {
	const glm::ivec3& d = glm::abs(_pendingExtractionSortPosition - sortPos);
	const int allowedDelta = 3 * _meshSize->intVal();
	if (d.x < allowedDelta && d.z < allowedDelta) {
		return;
	}
	_pendingExtractionSortPosition = sortPos;
	_pendingExtraction.setComparator(CloseToPoint(sortPos));
}

bool WorldMgr::allowReExtraction(const glm::ivec3& pos) {
	const glm::ivec3& gridPos = meshPos(pos);
	return _positionsExtracted.erase(gridPos) != 0;
}

bool WorldMgr::init(uint32_t volumeMemoryMegaBytes, uint16_t chunkSideLength) {
	_threadPool.init();
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);
	_volumeData = new voxel::PagedVolume(_pager.get(), volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);

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
		const glm::ivec3 maxs(pos.x + size.x - 1, pos.y + size.y - 2, pos.z + size.z - 1);
		const voxel::Region region(mins, maxs);
		// these numbers are made up mostly by try-and-error - we need to revisit them from time to time to prevent extra mem allocs
		// they also heavily depend on the size of the mesh region we extract
		const int opaqueFactor = 16;
		const int opaqueVertices = region.getWidthInVoxels() * region.getDepthInVoxels() * opaqueFactor;
		const int waterVertices = region.getWidthInVoxels() * region.getDepthInVoxels();
		ChunkMeshes data(opaqueVertices, opaqueVertices, waterVertices, waterVertices);
		voxel::extractAllCubicMesh(_volumeData, region,
				&data.opaqueMesh, &data.waterMesh,
				voxel::IsQuadNeeded(), voxel::IsWaterQuadNeeded(),
				voxel::MAX_WATER_HEIGHT);
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
	delete _volumeData;
	_volumeData = nullptr;
}

void WorldMgr::stats(int& meshes, int& extracted, int& pending) const {
	extracted = _positionsExtracted.size();
	pending = _pendingExtraction.size();
	meshes = _extracted.size();
}

int WorldMgr::findWalkableFloor(const glm::vec3& position, float maxDistanceY) const {
	const voxel::VoxelType type = material(position.x, position.y, position.z);
	int y = voxel::NO_FLOOR_FOUND;
	if (voxel::isEnterable(type)) {
		raycast(position, glm::down, (glm::min)(maxDistanceY, position.y), [&] (const voxel::PagedVolume::Sampler& sampler) {
			voxel::VoxelType mat = sampler.voxel().getMaterial();
			if (!voxel::isEnterable(mat)) {
				y = sampler.position().y + 1;
				return false;
			}
			return true;
		});
	} else {
		raycast(position, glm::up, (glm::min)(maxDistanceY, (float)voxel::MAX_HEIGHT - position.y), [&] (const voxel::PagedVolume::Sampler& sampler) {
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
	return glm::ivec3(s, voxel::MAX_MESH_CHUNK_HEIGHT, s);
}

voxel::VoxelType WorldMgr::material(int x, int y, int z) const {
	const voxel::Voxel& voxel = _volumeData->voxel(x, y, z);
	return voxel.getMaterial();
}

}
