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

bool PLYFormat::saveMesh(const voxel::Mesh& mesh, const io::FilePtr &file, float scale, bool quad) {
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
	stream.addStringFormat(false, "property float x\n");
	stream.addStringFormat(false, "property float z\n");
	stream.addStringFormat(false, "property float y\n");
	stream.addStringFormat(false, "property uchar red\n");
	stream.addStringFormat(false, "property uchar green\n");
	stream.addStringFormat(false, "property uchar blue\n");

	int faces;
	if (quad) {
		faces = ni / 6;
	} else {
		faces = ni / 3;
	}

	stream.addStringFormat(false, "element face %i\n", faces);
	stream.addStringFormat(false, "property list uchar uint vertex_indices\n");

	stream.addStringFormat(false, "end_header\n");

	const MaterialColorArray& colors = getMaterialColors();
	for (int i = 0; i < nv; ++i) {
		const voxel::VoxelVertex& v = vertices[i];
		const glm::vec4& color = colors[v.colorIndex];
		stream.addStringFormat(false, "%f %f %f %u %u %u\n",
			(float)v.position.x * scale, (float)v.position.y * scale, -(float)v.position.z * scale,
			(uint8_t)(color.r * 255.0f), (uint8_t)(color.g * 255.0f), (uint8_t)(color.b * 255.0f));
	}

	if (quad) {
		for (int i = 0; i < ni; i += 3) {
			const voxel::IndexType one   = indices[i + 0];
			const voxel::IndexType two   = indices[i + 1];
			const voxel::IndexType three = indices[i + 2];
			stream.addStringFormat(false, "3 %i %i %i\n", (int)one, (int)two, (int)three);
		}
	} else {
		for (int i = 0; i < ni; i += 6) {
			const voxel::IndexType one   = indices[i + 0];
			const voxel::IndexType two   = indices[i + 1];
			const voxel::IndexType three = indices[i + 2];
			const voxel::IndexType four  = indices[i + 5];
			stream.addStringFormat(false, "4 %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
		}
	}
	return true;
}

}
