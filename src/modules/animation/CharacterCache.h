/**
 * @file
 */

#pragma once

#include "voxelformat/MeshCache.h"
#include "Vertex.h"
#include "CharacterSettings.h"
#include "CharacterMeshType.h"
#include "voxel/polyvox/Mesh.h"
#include <memory>

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c Character
 */
class CharacterCache : public voxelformat::MeshCache {
private:
	bool load(const char *basePath, const char *filename, CharacterMeshType meshType, const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]);
	bool load(const char *basePath, const char *filename, CharacterMeshType meshType, voxel::VoxelVolumes& volumes);
	bool loadGlider(const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]);
public:
	bool getCharacterVolumes(const CharacterSettings& settings, voxel::VoxelVolumes& volumes);
	bool getCharacterMeshes(const CharacterSettings& settings, const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]);
	bool getCharacterModel(const CharacterSettings& settings, Vertices& vertices, Indices& indices);
	bool getItemModel(const char *itemName, Vertices& vertices, Indices& indices);
};

using CharacterCachePtr = std::shared_ptr<CharacterCache>;

}
