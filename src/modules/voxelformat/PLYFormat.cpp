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

bool PLYFormat::saveMeshes(const Meshes& meshes, const io::FilePtr &file, float scale, bool quad, bool withColor, bool withTexCoords) {
	io::FileStream stream(file);

	stream.addStringFormat(false, "ply\nformat ascii 1.0\n");
	stream.addStringFormat(false, "comment github.com/mgerhardy/engine\n");
	stream.addStringFormat(false, "comment TextureFile palette-%s.png\n", voxel::getDefaultPaletteName());

	int elements = 0;
	int indices = 0;
	for (const auto& meshExt : meshes) {
		const voxel::Mesh& mesh = *meshExt.mesh;
		elements += mesh.getNoOfVertices();
		indices += mesh.getNoOfIndices();
	}

	stream.addStringFormat(false, "element vertex %i\n", elements);
	stream.addStringFormat(false, "property float x\n");
	stream.addStringFormat(false, "property float z\n");
	stream.addStringFormat(false, "property float y\n");
	if (withTexCoords) {
		stream.addStringFormat(false, "property float s\n");
		stream.addStringFormat(false, "property float t\n");
	}
	if (withColor) {
		stream.addStringFormat(false, "property uchar red\n");
		stream.addStringFormat(false, "property uchar green\n");
		stream.addStringFormat(false, "property uchar blue\n");
	}

	int faces;
	if (quad) {
		faces = indices / 6;
	} else {
		faces = indices / 3;
	}

	stream.addStringFormat(false, "element face %i\n", faces);
	stream.addStringFormat(false, "property list uchar uint vertex_indices\n");
	stream.addStringFormat(false, "end_header\n");

	const MaterialColorArray& colors = getMaterialColors();
	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)colors.size();
	// it is only 1 pixel high - sample the middle
	const float v1 = 0.5f;

	for (const auto& meshExt : meshes) {
		const voxel::Mesh& mesh = *meshExt.mesh;
		const int nv = mesh.getNoOfVertices();
		const voxel::VoxelVertex* vertices = mesh.getRawVertexData();

		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex& v = vertices[i];
			const glm::vec4& color = colors[v.colorIndex];
			stream.addStringFormat(false, "%f %f %f",
				(float)v.position.x * scale, (float)v.position.y * scale, -(float)v.position.z * scale);
			if (withTexCoords) {
				const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
				stream.addStringFormat(false, " %f %f", u, v1);
			}
			if (withColor) {
				stream.addStringFormat(false, " %u %u %u",
					(uint8_t)(color.r * 255.0f), (uint8_t)(color.g * 255.0f), (uint8_t)(color.b * 255.0f));
			}
			stream.addStringFormat(false, "\n");
		}
	}

	int idxOffset = 0;
	for (const auto& meshExt : meshes) {
		const voxel::Mesh& mesh = *meshExt.mesh;
		const int ni = mesh.getNoOfIndices();
		const int nv = mesh.getNoOfVertices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		const voxel::IndexType* indices = mesh.getRawIndexData();
		if (quad) {
			for (int i = 0; i < ni; i += 6) {
				const uint32_t one   = idxOffset + indices[i + 0];
				const uint32_t two   = idxOffset + indices[i + 1];
				const uint32_t three = idxOffset + indices[i + 2];
				const uint32_t four  = idxOffset + indices[i + 5];
				stream.addStringFormat(false, "4 %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
			}
		} else {
			for (int i = 0; i < ni; i += 3) {
				const uint32_t one   = idxOffset + indices[i + 0];
				const uint32_t two   = idxOffset + indices[i + 1];
				const uint32_t three = idxOffset + indices[i + 2];
				stream.addStringFormat(false, "3 %i %i %i\n", (int)one, (int)two, (int)three);
			}
		}
		idxOffset += nv;
	}
	return true;
}

}
