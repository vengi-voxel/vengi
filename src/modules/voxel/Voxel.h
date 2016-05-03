#pragma once

#include "polyvox/Voxel.h"
#include "polyvox/Mesh.h"
#include "polyvox/Vertex.h"
#include "polyvox/CubicSurfaceExtractor.h"

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT 100
#define MAX_MOUNTAIN_HEIGHT (MAX_TERRAIN_HEIGHT + 60)
#define MAX_WATER_HEIGHT 10

constexpr Voxel createVoxel(VoxelType type) {
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

inline bool isLeaves(VoxelType material) {
	return material >= Leaves1 && material <= Leaves10;
}

inline bool isWood(VoxelType material) {
	return material == Wood1;
}

inline bool isGrass(VoxelType material) {
	return material == Grass1;
}

inline bool isRock(VoxelType material) {
	return material == Rock1 ||material == Rock2 || material == Rock3 || material == Rock4;
}

inline bool isSand(VoxelType material) {
	return material == Sand1 ||material == Sand2 || material == Sand3 || material == Sand4;
}

inline bool isDirt(VoxelType material) {
	return material == Dirt1 ||material == Dirt2 || material == Dirt3 || material == Dirt4;
}

inline bool isFloor(VoxelType material) {
	return isRock(material) || isDirt(material) || isSand(material) || isGrass(material);
}

}
