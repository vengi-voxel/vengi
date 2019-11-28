/**
 * @file
 */

#include "CharacterCache.h"
#include "CharacterSkeleton.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Common.h"
#include "voxelformat/Loader.h"
#include "voxelformat/VoxFileFormat.h"

namespace animation {

bool CharacterCache::loadGlider(const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]) {
	const char *fullPath = "models/glider.vox";
	voxel::Mesh& mesh = cacheEntry(fullPath);
	if (mesh.getNoOfVertices() > 0) {
		meshes[std::enum_value(CharacterMeshType::Glider)] = &mesh;
		return true;
	}
	if (loadMesh(fullPath, mesh)) {
		meshes[std::enum_value(CharacterMeshType::Glider)] = &mesh;
		return true;
	}
	meshes[std::enum_value(CharacterMeshType::Glider)] = nullptr;
	Log::error("Failed to load glider");
	return false;
}

bool CharacterCache::getCharacterModel(const CharacterSettings& settings, Vertices& vertices, Indices& indices) {
	return getBoneModel(settings, vertices, indices, [&] (const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]) {
		return loadGlider(meshes);
	});
}

bool CharacterCache::getItemModel(const char *itemName, Vertices& vertices, Indices& indices) {
	char fullPath[128];
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "models/items/%s.vox", itemName)) {
		Log::error("Failed to initialize the item path buffer. Can't load item %s.", itemName);
		return false;
	}
	return getModel(fullPath, BoneId::Tool, vertices, indices);
}

}
