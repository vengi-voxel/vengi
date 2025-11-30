/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxelformat/private/mesh/Mesh.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

class Polygon {
private:
	core::Buffer<glm::vec2> _uvs;
	core::Buffer<glm::vec3> _vertices;
	core::Buffer<color::RGBA> _colors;
	MeshMaterialIndex _materialIdx = -1;
	void addTriangle(MeshTriCollection &tris, int idx0, int idx1, int idx2) const;

public:
	Polygon &setMaterialIndex(MeshMaterialIndex materialIdx);
	Polygon &addVertex(const glm::vec3 &vertex, const glm::vec2 &uv, color::RGBA color = color::RGBA(0, 0, 0));
	bool toTris(MeshTriCollection &tris) const;
	bool toTris(Mesh &mesh) const;

	glm::vec3 center() const;
	size_t size() const;
	glm::vec3 vertex(int idx) const;
	void setVertex(int idx, const glm::vec3 &vertex);
};

} // namespace voxelformat
