#pragma once

#include <glm/glm.hpp>
#include "CubicSurfaceExtractor.h"
#include "MaterialDensityPair.h"
#include "Mesh.h"
#include "Vertex.h"
#include <vector>

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT MAX_HEIGHT / 2

// density 0 - 255 (8 bits)
// material types 0 - 255 (8 bits)
typedef uint8_t VoxelType;
typedef MaterialDensityPair88 Voxel;

inline Voxel createVoxel(VoxelType type) {
	return Voxel(type, Voxel::getMaxDensity());
}

typedef CubicVertex<voxel::Voxel> VoxelVertex;
typedef Mesh<VoxelVertex> CubicMesh;
typedef Vertex<voxel::Voxel> VoxelVertexDecoded;
typedef Mesh<VoxelVertexDecoded, typename CubicMesh::IndexType> DecodedMesh;

constexpr int MAX_VOXEL_LOD = 1;
static_assert(MAX_VOXEL_LOD >= 1, "MAX_LODS might not be smaller than 1");

struct DecodedMeshData {
	DecodedMesh mesh[MAX_VOXEL_LOD];
	// translation on the x and z axis
	glm::ivec3 translation;
	int numLods;
};

static const VoxelType Invalid = -1;
// this must be 0
static const VoxelType Air = 0;
static const VoxelType Grass = 1;
static const VoxelType Wood = 2;
static const VoxelType Leaves1 = 3;
static const VoxelType Leaves2 = 4;
static const VoxelType Leaves3 = 5;
static const VoxelType Leaves4 = 6;
static const VoxelType Leaves5 = 7;
static const VoxelType Leaves6 = 8;
static const VoxelType Leaves7 = 9;
static const VoxelType Leaves8 = 10;
static const VoxelType Leaves9 = 11;
static const VoxelType Leaves10 = 12;
static const VoxelType Rock = 13;
static const VoxelType Cloud = 14;
static const VoxelType Water = 15;

inline bool isFloor(VoxelType material) {
	return material == Rock || material == Grass;
}

}
