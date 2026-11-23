/**
 * @file
 */

#include "Mesh.h"
#include <limits>
#include "math/Vec3f.h"

namespace voxelformat {

void Mesh::clearAfterTriangulation() {
	indices.release();
	vertices.release();
	polygons.release();
}

void Mesh::reserveAdditionalTris(size_t numTris) {
	vertices.reserve(vertices.size() + numTris * 3);
	indices.reserve(indices.size() + numTris * 3);
}

void Mesh::addTriangle(const voxelformat::MeshTri &tri) {
	const math::Vec3f &normal = tri.normal();
	MeshVertex vertex0;
	vertex0.pos = tri.vertex0();
	vertex0.uv = tri.uv0();
	vertex0.color = tri.color0();
	vertex0.normal = normal;
	vertex0.materialIdx = tri.materialIdx;
	MeshVertex vertex1;
	vertex1.pos = tri.vertex1();
	vertex1.uv = tri.uv1();
	vertex1.color = tri.color1();
	vertex1.normal = normal;
	vertex1.materialIdx = tri.materialIdx;
	MeshVertex vertex2;
	vertex2.pos = tri.vertex2();
	vertex2.uv = tri.uv2();
	vertex2.color = tri.color2();
	vertex2.normal = normal;
	vertex2.materialIdx = tri.materialIdx;
	indices.push_back((voxel::IndexType)vertices.size());
	vertices.emplace_back(vertex0);
	indices.push_back((voxel::IndexType)vertices.size());
	vertices.emplace_back(vertex1);
	indices.push_back((voxel::IndexType)vertices.size());
	vertices.emplace_back(vertex2);
}

bool Mesh::calculateAABB(math::Vec3f &mins, math::Vec3f &maxs) const {
	if (vertices.empty()) {
		mins = maxs = math::Vec3f(0.0f);
		return false;
	}
	mins = math::Vec3f(std::numeric_limits<float>::max());
	maxs = math::Vec3f(std::numeric_limits<float>::lowest());
	for (const MeshVertex &v : vertices) {
		mins = math::min(mins, v.pos);
		maxs = math::max(maxs, v.pos);
	}
	return true;
}

} // namespace voxelformat
