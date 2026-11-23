/**
 * @file
 */

#include "Polygon.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include <array>
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace voxelformat {

Polygon &Polygon::setMaterialIndex(MeshMaterialIndex materialIdx) {
	_materialIdx = materialIdx;
	return *this;
}

Polygon &Polygon::addVertex(const math::Vec3f &vertex, const math::Vec2f &uv, color::RGBA color) {
	_vertices.push_back(vertex);
	_uvs.push_back(uv);
	_colors.push_back(color);
	return *this;
}

static void projectPoints(const core::Buffer<math::Vec3f> &vertexCoords, core::Buffer<math::Vec2f> &pointsProjected,
						  const math::Vec3f &normal, const math::Vec3f &axis, const math::Vec3f &origin) {
	const math::Vec3f &perpendicularAxis = math::cross(normal, axis);
	for (const math::Vec3f &vc : vertexCoords) {
		const math::Vec3f &dir = vc - origin;
		pointsProjected.emplace_back(math::dot(dir, axis), math::dot(dir, perpendicularAxis));
	}
}

void Polygon::addTriangle(MeshTriCollection &tris, int idx0, int idx1, int idx2) const {
	MeshTri meshTri;
	meshTri.setVertices(_vertices[idx0], _vertices[idx1], _vertices[idx2]);
	meshTri.setUVs(_uvs[idx0], _uvs[idx1], _uvs[idx2]);
	meshTri.setColor(_colors[idx0], _colors[idx1], _colors[idx2]);
	meshTri.materialIdx = _materialIdx;
	tris.push_back(meshTri);
}

bool Polygon::toTris(Mesh &mesh) const {
	MeshTriCollection tris;
	if (!toTris(tris)) {
		return false;
	}
	mesh.indices.reserve(mesh.indices.size() + tris.size() * 3);
	mesh.vertices.reserve(mesh.vertices.size() + tris.size() * 3);
	for (const MeshTri &tri : tris) {
		mesh.addTriangle(tri);
	}
	return true;
}

bool Polygon::toTris(MeshTriCollection &tris) const {
	if (3 == _vertices.size()) {
		addTriangle(tris, 0, 1, 2);
		return true;
	} else if (4 == _vertices.size()) {
		addTriangle(tris, 0, 1, 2);
		addTriangle(tris, 2, 3, 0);
		return true;
	}

	math::Vec3f normal{0.0f};
	for (size_t i = 0; i < _vertices.size(); ++i) {
		const size_t j = (i + 1) % _vertices.size();
		const size_t k = (i + 2) % _vertices.size();
		const math::Vec3f &enter = _vertices[i];
		const math::Vec3f &cone = _vertices[j];
		const math::Vec3f &leave = _vertices[k];
		normal += math::normalize(math::cross(cone - enter, leave - enter));
	}
	normal = math::normalize(normal);

	const math::Vec3f &axis = math::normalize(_vertices[1] - _vertices[0]);
	core::Buffer<math::Vec2f> projectedPoints;
	projectedPoints.reserve(_vertices.size());
	projectPoints(_vertices, projectedPoints, normal, axis, _vertices[0]);

	using Point = std::array<double, 2>;
	using Points = std::vector<Point>;
	Points border;
	std::vector<Points> polygons;
	border.reserve(projectedPoints.size());
	for (const math::Vec2f &v : projectedPoints) {
		border.emplace_back(Point{v.x, v.y});
	}
	polygons.emplace_back(core::move(border));

	std::vector<voxel::IndexType> indices = mapbox::earcut<voxel::IndexType>(polygons);
	tris.reserve(tris.size() + indices.size() / 3);
	for (size_t i = 0; i < indices.size(); i += 3) {
		// earcut returns indices into the polygon's vertex array
		addTriangle(tris, indices[i], indices[i + 1], indices[i + 2]);
	}
	return true;
}

size_t Polygon::size() const {
	return _vertices.size();
}

math::Vec3f Polygon::vertex(int idx) const {
	return _vertices[idx];
}

void Polygon::setVertex(int idx, const math::Vec3f &vertex) {
	_vertices[idx] = vertex;
}

math::Vec3f Polygon::center() const {
	if (_vertices.empty()) {
		return math::Vec3f(0.0f);
	}

	math::Vec3f center = _vertices[0];
	for (int i = 1; i < (int)_vertices.size(); ++i) {
		center += _vertices[i];
	}
	center /= math::Vec3f(_vertices.size());
	return center;
}

} // namespace voxelformat
