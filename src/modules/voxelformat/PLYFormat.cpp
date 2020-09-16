/**
 * @file
 */

#include "PLYFormat.h"
#include "core/Log.h"
#include "core/Var.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "voxel/VoxelVertex.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"

namespace voxel {

bool PLYFormat::saveMesh(const voxel::Mesh& mesh, const io::FilePtr &file) {
	io::FileStream stream(file);

	const int nv = mesh.getNoOfVertices();
	const int ni = mesh.getNoOfIndices();
	if (ni % 3 != 0) {
		Log::error("Unexpected indices amount");
		return false;
	}
	const voxel::VoxelVertex* vertices = mesh.getRawVertexData();
	const voxel::IndexType* indices = mesh.getRawIndexData();

	stream.addStringFormat(false, "ply\nformat ascii 1.0\n");
	stream.addStringFormat(false, "comment github.com/mgerhardy/engine\n");

	stream.addStringFormat(false, "element vertex %i\n", nv);
	stream.addStringFormat(false, "property int x\n");
	stream.addStringFormat(false, "property int z\n");
	stream.addStringFormat(false, "property int y\n");
	stream.addStringFormat(false, "property uchar red\n");
	stream.addStringFormat(false, "property uchar green\n");
	stream.addStringFormat(false, "property uchar blue\n");

	stream.addStringFormat(false, "element face %i\n", ni / 3);
	stream.addStringFormat(false, "property list uchar uint vertex_indices\n");

	stream.addStringFormat(false, "end_header\n");

	const MaterialColorArray& colors = getMaterialColors();
	for (int i = 0; i < nv; ++i) {
		const voxel::VoxelVertex& v = vertices[i];
		const glm::vec4& color = colors[v.colorIndex];
		stream.addStringFormat(false, "%i %i %i %u %u %u\n",
			v.position.x, v.position.y, -v.position.z,
			(uint8_t)(color.r * 255.0f), (uint8_t)(color.g * 255.0f), (uint8_t)(color.b * 255.0f));
	}

	for (int i = 0; i < ni; i += 3) {
		const voxel::IndexType one   = indices[i + 0];
		const voxel::IndexType two   = indices[i + 1];
		const voxel::IndexType three = indices[i + 2];
		stream.addStringFormat(false, "3 %i %i %i\n", (int)one, (int)two, (int)three);
	}
	return true;
}

}
