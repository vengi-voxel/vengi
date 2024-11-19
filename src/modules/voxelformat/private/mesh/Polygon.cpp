/**
 * @file
 */

#include "Polygon.h"
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/private/mesh/MeshMaterial.h"

namespace voxelformat {

bool Polygon::toTris(MeshFormat::MeshTriCollection &tris) const {
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

} // namespace voxelformat
