/**
 * @file
 */

#include "PLYFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelformat {

bool PLYFormat::saveMeshes(const core::Map<int, int> &, const SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	int elements = 0;
	int indices = 0;
	for (const auto& meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			elements += (int)mesh.getNoOfVertices();
			indices += (int)mesh.getNoOfIndices();
		}
	}

	if (elements == 0 || indices == 0) {
		return false;
	}

	const core::String paletteName = core::string::replaceExtension(voxel::getPalette().name(), "png");
	stream.writeStringFormat(false, "ply\nformat ascii 1.0\n");
	stream.writeStringFormat(false, "comment version " PROJECT_VERSION " github.com/mgerhardy/vengi\n");
	stream.writeStringFormat(false, "comment TextureFile %s\n", paletteName.c_str());

	stream.writeStringFormat(false, "element vertex %i\n", elements);
	stream.writeStringFormat(false, "property float x\n");
	stream.writeStringFormat(false, "property float z\n");
	stream.writeStringFormat(false, "property float y\n");
	if (withTexCoords) {
		stream.writeStringFormat(false, "property float s\n");
		stream.writeStringFormat(false, "property float t\n");
	}
	if (withColor) {
		stream.writeStringFormat(false, "property uchar red\n");
		stream.writeStringFormat(false, "property uchar green\n");
		stream.writeStringFormat(false, "property uchar blue\n");
	}

	int faces;
	if (quad) {
		faces = indices / 6;
	} else {
		faces = indices / 3;
	}

	stream.writeStringFormat(false, "element face %i\n", faces);
	stream.writeStringFormat(false, "property list uchar uint vertex_indices\n");
	stream.writeStringFormat(false, "end_header\n");

	for (const auto& meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			const int nv = (int)mesh.getNoOfVertices();
			const voxel::VoxelVertex* vertices = mesh.getRawVertexData();
			const SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			KeyFrameIndex keyFrameIdx = 0;
			const SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const voxel::Palette &palette = graphNode.palette();

			for (int i = 0; i < nv; ++i) {
				const voxel::VoxelVertex& v = vertices[i];
				glm::vec3 pos;
				if (meshExt.applyTransform) {
					pos = transform.apply(v.position, meshExt.size);
				} else {
					pos = v.position;
				}
				pos *= scale;
				stream.writeStringFormat(false, "%f %f %f", pos.x, pos.y, pos.z);
				if (withTexCoords) {
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					stream.writeStringFormat(false, " %f %f", uv.x, uv.y);
				}
				if (withColor) {
					const core::RGBA color = palette.color(v.colorIndex);
					stream.writeStringFormat(false, " %u %u %u", color.r, color.g, color.b);
				}
				stream.writeStringFormat(false, "\n");
			}
		}
	}

	int idxOffset = 0;
	for (const auto& meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			const int ni = (int)mesh.getNoOfIndices();
			const int nv = (int)mesh.getNoOfVertices();
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
					stream.writeStringFormat(false, "4 %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
				}
			} else {
				for (int i = 0; i < ni; i += 3) {
					const uint32_t one   = idxOffset + indices[i + 0];
					const uint32_t two   = idxOffset + indices[i + 1];
					const uint32_t three = idxOffset + indices[i + 2];
					stream.writeStringFormat(false, "3 %i %i %i\n", (int)one, (int)two, (int)three);
				}
			}
			idxOffset += nv;
		}
	}
	return sceneGraph.firstPalette().save(paletteName.c_str());
}
}
