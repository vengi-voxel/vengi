/**
 * @file
 */

#include "OBJFormat.h"
#include "core/Log.h"
#include "core/Var.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "voxel/VoxelVertex.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"

namespace voxel {

bool OBJFormat::saveMesh(const voxel::Mesh& mesh, const io::FilePtr &file) {
	bool withColor = true;

	io::FileStream stream(file);

	const int nv = mesh.getNoOfVertices();
	const int ni = mesh.getNoOfIndices();
	if (ni % 3 != 0) {
		Log::error("Unexpected indices amount");
		return false;
	}
	const voxel::VoxelVertex* vertices = mesh.getRawVertexData();
	const voxel::IndexType* indices = mesh.getRawIndexData();

	stream.addStringFormat(false, "# github.com/mgerhardy/engine\n");

	const MaterialColorArray& colors = getMaterialColors();
	for (int i = 0; i < nv; ++i) {
		const voxel::VoxelVertex& v = vertices[i];
		if (withColor) {
			const glm::vec4& color = colors[v.colorIndex];
			stream.addStringFormat(false, "v %i %i %i %f %f %f\n",
				v.position.x, v.position.y, v.position.z, color.r, color.g, color.b);
		} else {
			stream.addStringFormat(false, "v %i %i %i\n",
				v.position.x, v.position.y, v.position.z);
		}
	}

	for (int i = 0; i < ni; i += 3) {
		const voxel::IndexType one   = indices[i + 0] + 1;
		const voxel::IndexType two   = indices[i + 1] + 1;
		const voxel::IndexType three = indices[i + 2] + 1;
		stream.addStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
	}
	return true;
}

}
