/**
 * @file
 */

#include "WorldMeshExtractor.h"
#include "core/concurrent/Concurrency.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/Constants.h"

namespace voxelrender {

WorldMeshExtractor::WorldMeshExtractor() :
		_threadPool(core::halfcpus(), "WorldMeshExtractor") {
}

bool WorldMeshExtractor::init(voxel::PagedVolume *volume) {
	_volume = volume;
	_threadPool.init();
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);
	for (size_t i = 0u; i < _threadPool.size(); ++i) {
		_threadPool.enqueue([this] () {extractScheduledMesh();});
	}
	return true;
}

void WorldMeshExtractor::shutdown() {
	_cancelThreads = true;
	_pendingExtraction.clear();
	_pendingExtraction.abortWait();
	_extracted.clear();
	_extracted.abortWait();
	_threadPool.shutdown();
	_positionsExtracted.clear();
	_extracted.clear();
	_volume = nullptr;
}

void WorldMeshExtractor::reset() {
	if (_volume != nullptr) {
		_volume->flushAll();
	}
	_extracted.clear();
	_positionsExtracted.clear();
	_pendingExtraction.clear();
}

bool WorldMeshExtractor::pop(ChunkMeshes& item) {
	return _extracted.pop(item);
}

glm::ivec3 WorldMeshExtractor::meshPos(const glm::ivec3& pos) const {
	const glm::vec3& size = meshSize();
	const int x = glm::floor(pos.x / size.x);
	const int y = glm::floor(pos.y / size.y);
	const int z = glm::floor(pos.z / size.z);
	return glm::ivec3(x * size.x, y * size.y, z * size.z);
}

glm::ivec3 WorldMeshExtractor::meshSize() const {
	const int s = _meshSize->intVal();
	return glm::ivec3(s, voxel::MAX_MESH_CHUNK_HEIGHT, s);
}

void WorldMeshExtractor::updateExtractionOrder(const glm::ivec3& sortPos) {
	const glm::ivec3& d = glm::abs(_pendingExtractionSortPosition - sortPos);
	const int allowedDelta = 3 * _meshSize->intVal();
	if (d.x < allowedDelta && d.z < allowedDelta) {
		return;
	}
	_pendingExtractionSortPosition = sortPos;
	_pendingExtraction.setComparator(CloseToPoint(sortPos));
}

bool WorldMeshExtractor::allowReExtraction(const glm::ivec3& pos) {
	const glm::ivec3& gridPos = meshPos(pos);
	return _positionsExtracted.erase(gridPos) != 0;
}

// Extract the surface for the specified region of the volume.
// The surface extractor outputs the mesh in an efficient compressed format which
// is not directly suitable for rendering.
bool WorldMeshExtractor::scheduleMeshExtraction(const glm::ivec3& p) {
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

void WorldMeshExtractor::extractScheduledMesh() {
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
		const int vertices = region.getWidthInVoxels() * region.getDepthInVoxels() * opaqueFactor;
		ChunkMeshes data(vertices, vertices);
		voxel::extractCubicMesh(_volume, region, &data.mesh, voxel::IsQuadNeeded());
		if (!data.mesh.isEmpty()) {
			_extracted.push(std::move(data));
		}
	}
}

}