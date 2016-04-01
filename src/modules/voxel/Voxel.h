#pragma once

#include <PolyVox/MaterialDensityPair.h>
#include <PolyVox/Mesh.h>
#include <PolyVox/Vertex.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <glm/glm.hpp>
#include <vector>

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT MAX_HEIGHT / 2

// density 0 - 255 (8 bits)
// material types 0 - 255 (8 bits)
typedef uint8_t VoxelType;
typedef PolyVox::MaterialDensityPair88 Voxel;

typedef PolyVox::CubicVertex<voxel::Voxel> VoxelVertex;
typedef PolyVox::Mesh<VoxelVertex> CubicMesh;
typedef PolyVox::Vertex<voxel::Voxel> VoxelVertexDecoded;
typedef PolyVox::Mesh<VoxelVertexDecoded, typename CubicMesh::IndexType> DecodedMesh;

struct DecodedMeshData {
	DecodedMesh mesh;
	// translation on the x and z axis
	glm::ivec2 translation;
};

// this must be 0
const VoxelType AIR = 0;
const VoxelType DIRT = 1;
const VoxelType GRASS = 2;
const VoxelType CLOUD = 3;
const VoxelType WATER = 4;
const VoxelType LEAVES = 5;
const VoxelType TRUNK = 6;
const VoxelType CLOUDS = 7;

}
