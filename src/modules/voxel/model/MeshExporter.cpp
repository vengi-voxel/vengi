#include "MeshExporter.h"
#include "core/Log.h"
#include "core/GLM.h"
#include <assimp/Exporter.hpp>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <SDL.h>

namespace voxel {

bool exportMesh(const Mesh* mesh, const char *filename) {
	const char* ext = SDL_strrchr(filename, '.');
	if (ext == nullptr) {
		return false;
	}
	++ext;
	if (*ext == '\0') {
		return false;
	}

	Assimp::Exporter exporter;
	Assimp::ExportProperties settings;
	// TODO: exporter.SetIOHandler(&iosystem);
	const size_t num = exporter.GetExportFormatCount();
	for (size_t i = 0; i < num; ++i) {
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		if (ext == desc->fileExtension) {
			Log::debug("Export %s to %s (%s)", ext, desc->id, desc->description);
			aiScene aiscene;
			aiMesh aimesh;
			aiMaterial aimaterial;

			aimesh.mNumVertices = mesh->getNoOfIndices();
			std::vector<aiVector3D> vertices(aimesh.mNumVertices);
			const voxel::Vertex* voxels = mesh->getRawVertexData();
			for (size_t i = 0; i < aimesh.mNumVertices; ++i) {
				const voxel::Vertex& v = voxels[i];
				vertices[i] = aiVector3D(v.position.x, v.position.y, v.position.z);
			}
			aimesh.mVertices = &vertices.front();
			for (int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
				// TODO:
				aimesh.mColors[i] = nullptr;
			}
			aiFace aiface;
			aiface.mNumIndices = (unsigned int)mesh->getNoOfIndices();
			aiface.mIndices = (unsigned int*)mesh->getRawIndexData();

			aimesh.mFaces = &aiface;
			aimesh.mNumFaces = 1;

			aiMesh* aimeshes[] = { &aimesh };
			aiscene.mMeshes = aimeshes;

			aiReturn ret = exporter.Export(&aiscene, desc->id, filename, 0, &settings);
			if (ret == aiReturn_SUCCESS) {
				return false;
			}
		} else {
			Log::debug("Don't export %s to %s (%s, '%s')", ext, desc->id, desc->description, desc->fileExtension);
		}
	}
	return false;
}

}
