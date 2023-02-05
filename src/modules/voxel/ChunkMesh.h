/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"

namespace voxel {

struct ChunkMesh {
	static constexpr int Meshes = 2;
	ChunkMesh(int vertices, int indices, bool mayGetResized = false)
		: mesh{{vertices, indices, mayGetResized}, {vertices, indices, mayGetResized}} {
	}
	ChunkMesh() : ChunkMesh(128, 128, true) {}
	voxel::Mesh mesh[Meshes];

	void setOffset(const glm::ivec3& offset) {
		for (int i = 0; i < Meshes; ++i) {
			mesh[i].setOffset(offset);
		}
	}
	void clear() {
		for (int i = 0; i < Meshes; ++i) {
			mesh[i].clear();
		}
	}
	bool isEmpty() const {
		for (int i = 0; i < Meshes; ++i) {
			if (!mesh[i].isEmpty()) {
				return false;
			}
		}
		return true;
	}
	void removeUnusedVertices() {
		for (int i = 0; i < Meshes; ++i) {
			mesh[i].removeUnusedVertices();
		}
	}
	void compressIndices() {
		for (int i = 0; i < Meshes; ++i) {
			mesh[i].compressedIndices();
		}
	}
};

} // namespace voxel
