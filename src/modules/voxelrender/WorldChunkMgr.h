/**
 * @file
 */

#pragma once

#include "math/Octree.h"
#include "WorldMeshExtractor.h"
#include "video/Camera.h"
#include "voxel/VoxelVertex.h"

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

	using Tree = math::Octree<ChunkBuffer *>;
	Tree _octree;
	static constexpr int MAX_CHUNKBUFFERS = 4096;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	int _maxAllowedDistance = -1;

	WorldMeshExtractor _meshExtractor;

	int getDistanceSquare(const glm::ivec3 &pos, const glm::ivec3 &pos2) const;

public:
	WorldChunkMgr();

	voxel::VertexArray _vertices;
	voxel::IndexArray _indices;

	void extractMesh(const glm::ivec3 &pos);
	void extractMeshes(const video::Camera &camera);

	void cull(const video::Camera &camera);
	void handleMeshQueue();

	void updateViewDistance(float viewDistance);
	bool init(voxel::PagedVolume* volume);
	void shutdown();
	void update(const glm::vec3& focusPos);
	void reset();
};

}