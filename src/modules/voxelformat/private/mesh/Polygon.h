/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "MeshMaterial.h"
#include "core/collection/Buffer.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

class Polygon {
private:
	core::Buffer<glm::vec2> _uvs;
	core::Buffer<glm::vec3> _vertices;
	core::Buffer<core::RGBA> _colors;
	MeshMaterialIndex _materialIdx;
	void addTriangle(MeshTriCollection &tris, int idx0, int idx1, int idx2) const;

public:
	Polygon &setMaterialIndex(MeshMaterialIndex materialIdx);
	Polygon &addVertex(const glm::vec3 &vertex, const glm::vec2 &uv, core::RGBA color = core::RGBA(0, 0, 0));
	bool toTris(MeshTriCollection &tris) const;

	glm::vec3 center() const;
	size_t size() const;
	glm::vec3 vertex(int idx) const;
	void setVertex(int idx, const glm::vec3 &vertex);
};

} // namespace voxelformat
