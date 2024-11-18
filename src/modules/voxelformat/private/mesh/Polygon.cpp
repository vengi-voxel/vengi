/**
 * @file
 */

#include "Polygon.h"
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/private/mesh/MeshMaterial.h"

namespace voxelformat {

bool Polygon::toTris(MeshFormat::TriCollection &tris) const {
	// TODO: VOXELFORMAT: mapbox::earcut
	if (_vertices.size() < 3) {
		return false;
	}
	if (_uvs.size() < 3) {
		return false;
	}

	core_assert(_vertices.size() == _uvs.size());
	core_assert(_vertices.size() == _colors.size());

	for (int i = 1; i < (int)_vertices.size() - 1; ++i) {
		MeshTri tri;
		tri.vertices[0] = _vertices[0];
		tri.vertices[1] = _vertices[i];
		tri.vertices[2] = _vertices[i + 1];
		tri.uv[0] = _uvs[0];
		tri.uv[1] = _uvs[i];
		tri.uv[2] = _uvs[i + 1];
		tri.color[0] = _colors[0];
		tri.color[1] = _colors[i];
		tri.color[2] = _colors[i + 1];
		tri.material = _material;
		tris.push_back(tri);
	}
	return true;
}

} // namespace voxelformat
