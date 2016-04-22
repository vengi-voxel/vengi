#pragma once

#include "polyvox/Voxel.h"
#include "polyvox/Mesh.h"
#include "polyvox/Vertex.h"
#include "polyvox/CubicSurfaceExtractor.h"

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT MAX_HEIGHT / 2

inline Voxel createVoxel(VoxelType type) {
	return Voxel(type);
}

typedef CubicVertex<voxel::Voxel> VoxelVertex;
typedef Mesh<VoxelVertex> CubicMesh;
typedef Vertex<voxel::Voxel> VoxelVertexDecoded;
typedef Mesh<VoxelVertexDecoded, typename CubicMesh::IndexType> DecodedMesh;

constexpr int MAX_VOXEL_LOD = 4;
static_assert(MAX_VOXEL_LOD >= 1, "MAX_LODS might not be smaller than 1");

struct DecodedMeshData {
	DecodedMesh mesh[MAX_VOXEL_LOD];
	// translation on the x and z axis
	glm::ivec3 translation;
	int numLods;
};

inline bool isFloor(VoxelType material) {
	return material == Rock || material == Grass;
}

}
