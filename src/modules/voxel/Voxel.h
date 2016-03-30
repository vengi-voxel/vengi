#pragma once

#include <PolyVox/MaterialDensityPair.h>
#include <PolyVox/Mesh.h>
#include <PolyVox/Vertex.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <glm/glm.hpp>
#include <vector>

namespace voxel {

// this must be 0
const int AIR = 0;
const int DIRT = 1;
const int GRASS = 2;
const int CLOUD = 3;
const int WATER = 4;
const int LEAVES = 5;
const int TRUNK = 6;
const int CLOUDS = 7;

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
