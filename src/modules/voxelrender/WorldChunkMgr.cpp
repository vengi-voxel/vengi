/**
 * @file
 */

#include "WorldChunkMgr.h"
#include "core/Trace.h"
#include "voxel/Constants.h"

namespace voxelrender {

WorldChunkMgr::WorldChunkMgr() : _octree({}, 30) {
}

void WorldChunkMgr::updateViewDistance(float viewDistance) {
	const glm::vec3 cullingThreshold(_meshExtractor.meshSize());
	const int maxCullingThreshold = core_max(cullingThreshold.x, cullingThreshold.z) * 4;
	_maxAllowedDistance = glm::pow(viewDistance + maxCullingThreshold, 2);
}

bool WorldChunkMgr::init(voxel::PagedVolume* volume) {
	return _meshExtractor.init(volume);
}

void WorldChunkMgr::shutdown() {
	_meshExtractor.shutdown();
}

void WorldChunkMgr::reset() {
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		chunkBuffer.inuse = false;
	}
	_meshExtractor.reset();
	_octree.clear();
	_activeChunkBuffers = 0;
}

void WorldChunkMgr::handleMeshQueue() {
	voxel::Mesh mesh;
	if (!_meshExtractor.pop(mesh)) {
		return;
	}
	// Now add the mesh to the list of meshes to render.
	core_trace_scoped(WorldRendererHandleMeshQueue);

	ChunkBuffer* freeChunkBuffer = nullptr;
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (freeChunkBuffer == nullptr && !chunkBuffer.inuse) {
			freeChunkBuffer = &chunkBuffer;
		}
		// check whether we update an existing one
		if (chunkBuffer.translation() == mesh.getOffset()) {
			freeChunkBuffer = &chunkBuffer;
			break;
		}
	}

	if (freeChunkBuffer == nullptr) {
		Log::warn("Could not find free chunk buffer slot");
		return;
	}

	freeChunkBuffer->mesh = std::move(mesh);
	freeChunkBuffer->_aabb = {freeChunkBuffer->mesh.mins(), freeChunkBuffer->mesh.maxs()};
	if (!_octree.insert(freeChunkBuffer)) {
		Log::warn("Failed to insert into octree");
	}
	if (!freeChunkBuffer->inuse) {
		freeChunkBuffer->inuse = true;
		++_activeChunkBuffers;
	}
}

static inline size_t transform(size_t indexOffset, const voxel::Mesh& mesh, voxel::VertexArray& verts, voxel::IndexArray& idxs) {
	const voxel::IndexArray& indices = mesh.getIndexVector();
	const size_t start = idxs.size();
	idxs.insert(idxs.end(), indices.begin(), indices.end());
	const size_t end = idxs.size();
	for (size_t i = start; i < end; ++i) {
		idxs[i] += indexOffset;
	}
	const voxel::VertexArray& vertices = mesh.getVertexVector();
	verts.insert(verts.end(), vertices.begin(), vertices.end());
	return vertices.size();
}

void WorldChunkMgr::cull(const video::Camera& camera) {
	core_trace_scoped(WorldRendererCull);
	_indices.clear();
	_vertices.clear();
	size_t indexOffset = 0;

	Tree::Contents contents;
	math::AABB<float> aabb = camera.frustum().aabb();
	aabb.shift(camera.forward() * -10.0f);
	_octree.query(math::AABB<int>(aabb.mins(), aabb.maxs()), contents);

	for (ChunkBuffer* chunkBuffer : contents) {
		core_trace_scoped(WorldRendererCullChunk);
		indexOffset += transform(indexOffset, chunkBuffer->mesh, _vertices, _indices);
	}
}

int WorldChunkMgr::getDistanceSquare(const glm::ivec3& pos, const glm::ivec3& pos2) const {
	const glm::ivec3 dist = pos - pos2;
	const int distance = dist.x * dist.x + dist.z * dist.z;
	return distance;
}

void WorldChunkMgr::update(const glm::vec3& focusPos) {
	_meshExtractor.updateExtractionOrder(focusPos);

	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		const int distance = getDistanceSquare(chunkBuffer.translation(), focusPos);
		if (distance < _maxAllowedDistance) {
			continue;
		}
		core_assert_always(_meshExtractor.allowReExtraction(chunkBuffer.translation()));
		chunkBuffer.inuse = false;
		--_activeChunkBuffers;
		_octree.remove(&chunkBuffer);
		Log::trace("Remove mesh from %i:%i", chunkBuffer.translation().x, chunkBuffer.translation().z);
	}
}

void WorldChunkMgr::extractMeshes(const video::Camera& camera) {
	core_trace_scoped(WorldRendererExtractMeshes);

	const float farplane = camera.farPlane();

	glm::vec3 mins = camera.position();
	mins.x -= farplane;
	mins.y = 0;
	mins.z -= farplane;

	glm::vec3 maxs = camera.position();
	maxs.x += farplane;
	maxs.y = voxel::MAX_HEIGHT;
	maxs.z += farplane;

	_octree.visit(mins, maxs, [&] (const glm::ivec3& mins, const glm::ivec3& maxs) {
		return !_meshExtractor.scheduleMeshExtraction(mins);
	}, glm::vec3(_meshExtractor.meshSize()));
}

void WorldChunkMgr::extractMesh(const glm::ivec3& pos) {
	_meshExtractor.scheduleMeshExtraction(pos);
}

}