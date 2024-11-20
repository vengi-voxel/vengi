/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

class Polygon {
private:
	core::DynamicArray<glm::vec2> _uvs;
	core::DynamicArray<glm::vec3> _vertices;
	core::DynamicArray<core::RGBA> _colors;
	MeshMaterialPtr _material;
	image::TextureWrap _wrapS = image::TextureWrap::Repeat;
	image::TextureWrap _wrapT = image::TextureWrap::Repeat;

public:
	Polygon &setMaterial(const MeshMaterialPtr &material);
	Polygon &setWrapS(image::TextureWrap wrapS);
	Polygon &setWrapT(image::TextureWrap wrapT);
	Polygon &addVertex(const glm::vec3 &vertex, const glm::vec2 &uv, core::RGBA color = core::RGBA(0, 0, 0));
	glm::vec2 uv(int x, int y) const;
	bool toTris(MeshFormat::MeshTriCollection &tris) const;
};

} // namespace voxelformat
