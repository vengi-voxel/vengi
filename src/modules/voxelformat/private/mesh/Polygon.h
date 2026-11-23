/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxelformat/private/mesh/Mesh.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace voxelformat {

class Polygon {
private:
	core::Buffer<math::Vec2f> _uvs;
	core::Buffer<math::Vec3f> _vertices;
	core::Buffer<color::RGBA> _colors;
	MeshMaterialIndex _materialIdx = -1;
	void addTriangle(MeshTriCollection &tris, int idx0, int idx1, int idx2) const;

public:
	Polygon &setMaterialIndex(MeshMaterialIndex materialIdx);
	Polygon &addVertex(const math::Vec3f &vertex, const math::Vec2f &uv, color::RGBA color = color::RGBA(0, 0, 0));
	bool toTris(MeshTriCollection &tris) const;
	bool toTris(Mesh &mesh) const;

	math::Vec3f center() const;
	size_t size() const;
	math::Vec3f vertex(int idx) const;
	void setVertex(int idx, const math::Vec3f &vertex);
};

} // namespace voxelformat
