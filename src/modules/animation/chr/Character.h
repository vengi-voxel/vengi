/**
 * @file
 */

#pragma once

#include "animation/AnimationEntity.h"
#include "CharacterSkeleton.h"
#include "CharacterSettings.h"
#include "CharacterCache.h"
#include "attrib/ShadowAttributes.h"
#include "stock/Inventory.h"
#include <stdint.h>

namespace animation {

/**
 * @brief Handles the loading and creation of the meshes and the bones regarding
 * the given @c CharacterSettings
 */
class Character : public AnimationEntity {
protected:
	CharacterSkeleton _skeleton;
	// the offset where the tool starts at
	size_t _toolVerticesOffset = 0u;
	// the offset where the tool starts at
	size_t _toolIndicesOffset = 0u;
	Vertices _toolVertices;
	Indices _toolIndices;
	stock::ItemId _toolId = (stock::ItemId)-1;
	ToolAnimationType _toolAnim = ToolAnimationType::None;
	CharacterSettings _settings;
public:
	/**
	 * @brief Initializes the character settings via (optional) lua script.
	 * @return @c true if the initialization was successful, @c false otherwise.
	 */
	bool init(const CharacterCachePtr& cache, const std::string& luaString = "");
	void shutdown();
	bool initMesh(const CharacterCachePtr& cache);
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
	 * @param[in] inv The stock::Inventory object to query the active items
	 */
	bool updateTool(const CharacterCachePtr& cache, const stock::Inventory& inv);
	const Skeleton& skeleton() const override;
};

}
