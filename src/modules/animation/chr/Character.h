/**
 * @file
 */

#pragma once

#include "animation/AnimationEntity.h"
#include "CharacterSkeleton.h"
#include "animation/AnimationCache.h"
#include "attrib/ShadowAttributes.h"
#include "stock/Stock.h"
#include <stdint.h>

namespace animation {

/**
 * @brief Handles the loading and creation of the meshes and the bones regarding
 * the given @c CharacterSettings
 */
class Character : public AnimationEntity {
protected:
	CharacterSkeleton _skeleton;
	CharacterSkeletonAttribute _attributes;

	// the offset where the tool starts at
	size_t _toolVerticesOffset = 0u;
	// the offset where the tool starts at
	size_t _toolIndicesOffset = 0u;
	Vertices _toolVertices;
	Indices _toolIndices;
	stock::ItemId _toolId = (stock::ItemId)-1;
	ToolAnimationType _toolAnim = ToolAnimationType::None;

	bool loadGlider(const AnimationCachePtr& cache, const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]);
public:
	/**
	 * @brief Initializes the character settings via (optional) lua script.
	 * @return @c true if the initialization was successful, @c false otherwise.
	 */
	bool init(const AnimationCachePtr& cache, const std::string& luaString = "");
	void shutdown();
	bool initMesh(const AnimationCachePtr& cache);
	/**
	 * @note Updating the settings without updating the mesh afterwards is pointless.
	 */
	bool initSettings(const std::string& luaString);
	/**
	 * @brief Update the bone states and the tool vertices from the given inventory
	 * @param[in] dt The delta time since the last call
	 * @param[in] attrib @c attrib::ShadowAttributes to get the character values
	 * from
	 */
	void update(uint64_t dt, const attrib::ShadowAttributes& attrib);
	/**
	 * @brief Updates the vertices and indices buffer whenever the character switched the active tool
	 * @param[in] cache The cache that is used to resolve the item models
	 * @param[in] stock The stock::Stock object to query the active items
	 */
	bool updateTool(const AnimationCachePtr& cache, const stock::Stock& stock);
	const Skeleton& skeleton() const override;
	CharacterSkeletonAttribute& skeletonAttributes();
	const CharacterSkeletonAttribute& skeletonAttributes() const;
};

inline CharacterSkeletonAttribute& Character::skeletonAttributes() {
	return _attributes;
}

inline const CharacterSkeletonAttribute& Character::skeletonAttributes() const {
	return _attributes;
}

}
