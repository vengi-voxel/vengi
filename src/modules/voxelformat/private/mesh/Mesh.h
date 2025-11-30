/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"
#include "voxelformat/private/mesh/MeshTri.h"

namespace voxelformat {

struct MeshVertex {
	glm::vec3 pos{0.0f};
	glm::vec2 uv{0.0f};
	color::RGBA color{0};
	glm::vec3 normal{0.0f};
	MeshMaterialIndex materialIdx = -1;
};
struct Mesh {
	core::DynamicArray<MeshVertex> vertices;
	voxel::IndexArray indices;
	/**
	 * @c MeshVertex instances have a @c MeshMaterialIndex pointing into this array
	 */
	MeshMaterialArray materials;
	/**
	 * polygons are just indices into the vertices array
	 * they must be triangulated before they are voxelized.
	 * @sa MeshFormat::triangulatePolygons()
	 */
	core::DynamicArray<voxel::IndexArray> polygons;

	void clearAfterTriangulation();
	void reserveAdditionalTris(size_t numTris);
	// helper function to add a triangle to the mesh - better add it directly
	void addTriangle(const voxelformat::MeshTri &tri);
};

} // namespace voxelformat
