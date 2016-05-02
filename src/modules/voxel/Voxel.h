#pragma once

#include "polyvox/Voxel.h"
#include "polyvox/Mesh.h"
#include "polyvox/Vertex.h"
#include "polyvox/CubicSurfaceExtractor.h"

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT 100
#define MAX_WATER_HEIGHT 10

inline Voxel createVoxel(VoxelType type) {
	return Voxel(type);
}

typedef Mesh<CubicVertex> CubicMesh;
typedef Mesh<Vertex> DecodedMesh;

constexpr int MAX_VOXEL_LOD = 2;
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

inline bool isLeaves(VoxelType material) {
	return material == Leaves1 || material == Leaves10;
}

inline bool isWood(VoxelType material) {
	return material == Wood;
}

inline bool isRock(VoxelType material) {
	return material == Rock;
}

}
