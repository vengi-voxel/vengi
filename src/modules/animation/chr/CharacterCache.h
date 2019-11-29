/**
 * @file
 */

#pragma once

#include "animation/AnimationCache.h"
#include "animation/Vertex.h"
#include "CharacterSettings.h"

namespace animation {

/**
 * @brief Cache @c voxel::Mesh instances for @c Character
 */
class CharacterCache : public AnimationCache {
private:
	bool loadGlider(const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]);
public:
	bool getCharacterModel(const AnimationSettings& settings, Vertices& vertices, Indices& indices);
	bool getItemModel(const char *itemName, Vertices& vertices, Indices& indices);
};

using CharacterCachePtr = std::shared_ptr<CharacterCache>;

}
