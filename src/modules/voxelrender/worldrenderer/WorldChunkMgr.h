/**
 * @file
 */

#pragma once

#include "math/Octree.h"
#include "WorldMeshExtractor.h"
#include "video/Camera.h"
#include "voxel/VoxelVertex.h"
#include <future>

namespace voxelrender {

class WorldChunkMgr {
protected:
	struct ChunkBuffer {
		bool inuse = false;
		math::AABB<int> _aabb = {glm::zero<glm::ivec3>(), glm::zero<glm::ivec3>()};
		voxel::Mesh mesh;

		/**
		 * This is the world position. Not the render positions. There is no scale
		 * applied here.
		 */
		inline const glm::ivec3 &translation() const {
			return mesh.getOffset();
		}

		/**
		 * This is the render aabb. There might be a scale applied here. So the mins of
		 * the AABB might not be at the position given by @c translation()
		 */
		inline const math::AABB<int> &aabb() const {
			return _aabb;
		}
	};

	struct BufferIndices {
		int current;
		int next;
	};
	std::future<BufferIndices> _cullResult;

	using Tree = math::Octree<ChunkBuffer *>;
	Tree _octree;
	static constexpr int MAX_CHUNKBUFFERS = 512;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	int _maxAllowedDistance = -1;

	WorldMeshExtractor _meshExtractor;
	core::ThreadPool &_threadPool;

	int distance2(const glm::ivec3 &pos, const glm::ivec3 &pos2) const;

	static constexpr int BufferCount = 2;
	int _nextBuffer = 0u;
	int _currentBuffer = 0u;
	voxel::VertexArray _vertices[BufferCount];
	voxel::IndexArray _indices[BufferCount];

	void cull(const video::Camera &camera);
	void handleMeshQueue();

public:
	WorldChunkMgr(core::ThreadPool& threadPool);

	const voxel::VertexArray& vertices() const;
	const voxel::IndexArray& indices() const;

	void extractMesh(const glm::ivec3 &pos);
	void extractMeshes(const video::Camera &camera);
	void extractScheduledMesh();

	void update(const video::Camera &camera, const glm::vec3& focusPos);

	void updateViewDistance(float viewDistance);
	bool init(voxel::PagedVolume* volume);
	void shutdown();
	void reset();
};

inline const voxel::VertexArray& WorldChunkMgr::vertices() const {
	return _vertices[_currentBuffer];
}

inline const voxel::IndexArray& WorldChunkMgr::indices() const {
	return _indices[_currentBuffer];
}

}
