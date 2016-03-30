#pragma once

#include <PolyVox/MaterialDensityPair.h>
#include <PolyVox/Mesh.h>
#include <PolyVox/Vertex.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <glm/glm.hpp>
#include <vector>

namespace voxel {

#define MAX_HEIGHT 255

// this must be 0
const uint8_t AIR = 0;
const uint8_t DIRT = 1;
const uint8_t GRASS = 2;
const uint8_t CLOUD = 3;
const uint8_t WATER = 4;
const uint8_t LEAVES = 5;
const uint8_t TRUNK = 6;
const uint8_t CLOUDS = 7;

// density 0 - 255 (8 bits)
// material types 0 - 255 (8 bits)
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

}
