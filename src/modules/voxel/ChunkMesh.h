/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"

namespace voxel {

struct ChunkMesh {
	ChunkMesh(int vertices, int indices, bool mayGetResized = false)
		: mesh(vertices, indices, mayGetResized), meshT(vertices, indices, mayGetResized) {
	}
	ChunkMesh() : ChunkMesh(128, 128, true) {}
	voxel::Mesh mesh;
	voxel::Mesh meshT;

	void setOffset(const glm::ivec3& offset) {
		mesh.setOffset(offset);
		meshT.setOffset(offset);
	}
	void clear() {
		mesh.clear();
		meshT.clear();
	}
	bool isEmpty() const {
		return mesh.isEmpty() && meshT.isEmpty();
	}
	void removeUnusedVertices() {
		mesh.removeUnusedVertices();
		meshT.removeUnusedVertices();
	}
	void compressIndices() {
		mesh.compressedIndices();
		meshT.compressedIndices();
	}
};

} // namespace voxel
