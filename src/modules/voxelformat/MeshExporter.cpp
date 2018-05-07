/**
 * @file
 */

#include "MeshExporter.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "voxel/MaterialColor.h"
#include <assimp/Exporter.hpp>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <SDL.h>

namespace voxel {

static bool doExport(Assimp::Exporter& exporter, const Mesh* mesh, const char *exporterId, const char *filename) {
	aiScene aiscene;
	aiMesh aimesh;
	aiNode airootnode;
	aiMaterial aimaterial;

	aimesh.mNumVertices = mesh->getNoOfVertices();
	aiVector3D* vertices = new aiVector3D[aimesh.mNumVertices];
	const voxel::VoxelVertex* voxels = mesh->getRawVertexData();
	for (size_t i = 0; i < aimesh.mNumVertices; ++i) {
		const voxel::VoxelVertex& v = voxels[i];
		vertices[i] = aiVector3D(v.position.x, v.position.y, v.position.z);
	}
	aimesh.mName = "";
	aimesh.mVertices = vertices;

	unsigned int* rawIndexData = const_cast<unsigned int*>(mesh->getRawIndexData());
	const unsigned int numIndices = (unsigned int)mesh->getNoOfIndices();
	aimesh.mNumFaces = numIndices / 3;
	aiFace* aifaces = aimesh.mFaces = new aiFace[aimesh.mNumFaces];
	core_assert(numIndices % 3 == 0);
	for (unsigned int i = 0, faceindex = 0; i < numIndices; i += 3, rawIndexData += 3, ++faceindex) {
		aiFace& aiface = aifaces[faceindex];
		aiface.mNumIndices = 3;
		aiface.mIndices = rawIndexData;
	}

	aiscene.mNumMaterials = 1;
	aiMaterial* aimaterials[] = {&aimaterial};
	aiscene.mMaterials = aimaterials;

	aiMesh* aimeshes[] = {&aimesh};
	aiscene.mMeshes = aimeshes;
	aiscene.mNumMeshes = 1;

	const MaterialColorArray& colorArray = getMaterialColors();
	aiColor4D* colors = new aiColor4D[aimesh.mNumVertices];
	for (unsigned int i = 0; i < aimesh.mNumVertices; ++i) {
		const voxel::VoxelVertex& v = voxels[i];
		const glm::vec4& c = colorArray[v.colorIndex];
		colors[i] = aiColor4D(c.r, c.g, c.b, c.a);
	}
	aimesh.mColors[0] = colors;

	airootnode.mName = "<DummyRootNode>";
	unsigned int aimeshindices[] = {0};
	airootnode.mMeshes = aimeshindices;
	airootnode.mNumMeshes = 1;

	aiscene.mRootNode = &airootnode;

	aiScene* exportScene = nullptr;
	aiCopyScene(&aiscene, &exportScene);
	core_assert(exportScene->mMeshes[0]->mColors[0] != colors);
	Assimp::ExportProperties settings;
	const aiReturn ret = exporter.Export(exportScene, exporterId, filename, 0u, &settings);

	aiscene.mMaterials = nullptr;
	aiscene.mRootNode = nullptr;
	aiscene.mMeshes = nullptr;
	airootnode.mMeshes = nullptr;
	for (unsigned int i = 0; i < aimesh.mNumFaces; ++i) {
		aifaces[i].mIndices = nullptr;
	}
	aimesh.mColors[0] = nullptr;
	aimesh.mVertices = nullptr;
	aimesh.mFaces = nullptr;

	delete[] vertices;
	delete[] colors;
	delete[] aifaces;

	aiFreeScene(exportScene);

	return ret == aiReturn_SUCCESS;
}

bool exportMesh(const Mesh* mesh, const char *filename) {
	const char* ext = SDL_strrchr(filename, '.');
	if (ext == nullptr) {
		Log::error("Could not determine the target format - no file extension was provided");
		return false;
	}
	++ext;
	if (*ext == '\0') {
		Log::error("Could not determine the target format - no file extension was provided");
		return false;
	}

	if (mesh->getNoOfVertices() == 0 || mesh->getNoOfIndices() == 0) {
		Log::error("Nothing to export - the voxel mesh is empty");
		return false;
	}

	Assimp::Exporter exporter;
	// TODO: exporter.SetIOHandler(&iosystem);
	const size_t num = exporter.GetExportFormatCount();
	for (size_t i = 0; i < num; ++i) {
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		if (!strcmp(ext, desc->fileExtension)) {
			Log::debug("Export %s to %s (%s)", ext, desc->id, desc->description);
			return doExport(exporter, mesh, desc->id, filename);
		} else {
			Log::debug("Don't export %s to %s (%s, '%s')", ext, desc->id, desc->description, desc->fileExtension);
		}
	}
	Log::error("Could not determine the target format - %s is not supported", ext);
	return false;
}

}
