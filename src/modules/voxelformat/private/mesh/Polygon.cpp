/**
 * @file
 */

#include "Polygon.h"
#include "core/collection/DynamicArray.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include <glm/geometric.hpp>

namespace voxelformat {

Polygon &Polygon::setMaterial(const MeshMaterialPtr &material) {
	_material = material;
	return *this;
}

Polygon &Polygon::addVertex(const glm::vec3 &vertex, const glm::vec2 &uv, core::RGBA color) {
	_vertices.push_back(vertex);
	_uvs.push_back(uv);
	_colors.push_back(color);
	return *this;
}

glm::vec2 Polygon::uv(int x, int y) const {
	if (_material && _material->texture) {
		return _material->texture->uv(x, y);
	}
	return {0.0f, 0.0f};
}

static void projectPoints(const core::DynamicArray<glm::vec3> &vertexCoords,
						  core::DynamicArray<glm::vec2> &pointsProjected, const glm::vec3 &normal,
						  const glm::vec3 &axis, const glm::vec3 &origin) {
	const glm::vec3 &perpendicularAxis = glm::cross(normal, axis);
	for (const glm::vec3 &vc : vertexCoords) {
		const glm::vec3 &dir = vc - origin;
		pointsProjected.emplace_back(glm::dot(dir, axis), glm::dot(dir, perpendicularAxis));
	}
}

void Polygon::addTriangle(MeshFormat::MeshTriCollection &tris, int idx0, int idx1, int idx2) const {
	MeshTri meshTri;
	meshTri.vertices[0] = _vertices[idx0];
	meshTri.vertices[1] = _vertices[idx1];
	meshTri.vertices[2] = _vertices[idx2];
	meshTri.uv[0] = _uvs[idx0];
	meshTri.uv[1] = _uvs[idx1];
	meshTri.uv[2] = _uvs[idx2];
	meshTri.color[0] = _colors[idx0];
	meshTri.color[1] = _colors[idx1];
	meshTri.color[2] = _colors[idx2];
	meshTri.material = _material;
	tris.push_back(meshTri);
}

bool Polygon::toTris(MeshFormat::MeshTriCollection &tris) const {
	if (3 == _vertices.size()) {
		addTriangle(tris, 0, 1, 2);
		return true;
	} else if (4 == _vertices.size()) {
		addTriangle(tris, 0, 1, 2);
		addTriangle(tris, 2, 3, 0);
		return true;
	}

	glm::vec3 normal{0.0f};
	for (size_t i = 0; i < _vertices.size(); ++i) {
		const size_t j = (i + 1) % _vertices.size();
		const size_t k = (i + 2) % _vertices.size();
		const glm::vec3 &enter = _vertices[i];
		const glm::vec3 &cone = _vertices[j];
		const glm::vec3 &leave = _vertices[k];
		normal += glm::normalize(glm::cross(cone - enter, leave - enter));
	}
	normal = glm::normalize(normal);

	const glm::vec3 &axis = glm::normalize(_vertices[1] - _vertices[0]);
	core::DynamicArray<glm::vec2> projectedPoints;
	projectedPoints.reserve(_vertices.size());
	projectPoints(_vertices, projectedPoints, normal, axis, _vertices[0]);

	using Point = std::array<double, 2>;
	using Points = std::vector<Point>;
	Points border;
	std::vector<Points> polygons;
	border.reserve(projectedPoints.size());
	for (const glm::vec2 &v : projectedPoints) {
		border.emplace_back(Point{v.x, v.y});
	}
	polygons.emplace_back(core::move(border));

	std::vector<voxel::IndexType> indices = mapbox::earcut<voxel::IndexType>(polygons);
	for (size_t i = 0; i < indices.size(); i += 3) {
		addTriangle(tris, i, i + 1, i + 2);
	}
	return true;
}

size_t Polygon::size() const {
	return _vertices.size();
}

glm::vec3 Polygon::vertex(int idx) const {
	return _vertices[idx];
}

void Polygon::setVertex(int idx, const glm::vec3 &vertex) {
	_vertices[idx] = vertex;
}

glm::vec3 Polygon::center() const {
	if (_vertices.empty()) {
		return glm::vec3(0.0f);
	}

	glm::vec3 center = _vertices[0];
	for (int i = 1; i < (int)_vertices.size(); ++i) {
		center += _vertices[i];
	}
	center /= glm::vec3(_vertices.size());
	return center;
}

} // namespace voxelformat
