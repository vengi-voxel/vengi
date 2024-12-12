/**
 * @file
 */

#include "Polygon.h"
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/private/mesh/MeshMaterial.h"

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

bool Polygon::toTris(MeshFormat::MeshTriCollection &tris) const {
	// TODO: VOXELFORMAT: mapbox::earcut - see PLYFormat::triangulatePolygons
	if (_vertices.size() < 3) {
		return false;
	}
	if (_uvs.size() < 3) {
		return false;
	}

	core_assert(_vertices.size() == _uvs.size());
	core_assert(_vertices.size() == _colors.size());

	for (int i = 1; i < (int)_vertices.size() - 1; ++i) {
		MeshTri meshTri;
		meshTri.vertices[0] = _vertices[0];
		meshTri.vertices[1] = _vertices[i];
		meshTri.vertices[2] = _vertices[i + 1];
		meshTri.uv[0] = _uvs[0];
		meshTri.uv[1] = _uvs[i];
		meshTri.uv[2] = _uvs[i + 1];
		meshTri.color[0] = _colors[0];
		meshTri.color[1] = _colors[i];
		meshTri.color[2] = _colors[i + 1];
		meshTri.material = _material;
		tris.push_back(meshTri);
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
