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
public:
	bool loadGlider(const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]);
};

using CharacterCachePtr = std::shared_ptr<CharacterCache>;

}
