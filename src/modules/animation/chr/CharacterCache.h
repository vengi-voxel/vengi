/**
 * @file
 */

#pragma once

#include "animation/AnimationCache.h"
#include "animation/Vertex.h"
#include "CharacterSettings.h"
#include "CharacterMeshType.h"

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c Character
 */
class CharacterCache : public AnimationCache<CharacterMeshType> {
private:
	bool loadGlider(const voxel::Mesh* (&meshes)[std::enum_value(CharacterMeshType::Max)]);
public:
	bool getCharacterModel(const CharacterSettings& settings, Vertices& vertices, Indices& indices);
	bool getItemModel(const char *itemName, Vertices& vertices, Indices& indices);
};

using CharacterCachePtr = std::shared_ptr<CharacterCache>;

}
