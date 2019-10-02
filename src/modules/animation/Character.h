/**
 * @file
 */

#pragma once

#include "Animation.h"
#include "Skeleton.h"
#include "CharacterSettings.h"
#include "CharacterCache.h"
#include "attrib/ShadowAttributes.h"
#include "stock/Inventory.h"
#include "voxel/polyvox/Mesh.h"
#include "Vertex.h"
#include <stdint.h>

namespace animation {

/**
 * @brief Handles the loading and creation of the meshes and the bones regarding
 * the given @c CharacterSettings
 */
class Character {
protected:
	CharacterSkeleton _skeleton;
	Animation _anim = Animation::Idle;
	Vertices _vertices;
	Indices _indices;
	// the offset where the tool starts at
	size_t _toolVerticesOffset = 0u;
	// the offset where the tool starts at
	size_t _toolIndicesOffset = 0u;
	Vertices _toolVertices;
	Indices _toolIndices;
	stock::ItemId _toolId = (stock::ItemId)-1;
	ToolAnimationType _toolAnim = ToolAnimationType::None;
	CharacterSettings _settings;

	bool load(const char *filename, voxel::Mesh& mesh, const glm::vec3& offset);
public:
	/**
	 * @brief Initializes the character settings via (optional) lua script.
	 * @return @c true if the initialization was successful, @c false otherwise.
	 */
	bool init(const CharacterCachePtr& cache, const std::string& luaString = "");
	void shutdown();
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

	void setAnimation(Animation animation);
	Animation animation() const;

	/**
	 * @brief The 'static' vertices of the character mesh where you have to apply
	 * the skeleton bones on
	 * @sa skeleton()
	 */
	const Vertices& vertices() const;
	/**
	 * @brief The 'static' indices of the character mesh
	 */
	const Indices& indices() const;

	/**
	 * @brief The skeleton data for the vertices
	 * @sa vertices()
	 */
	const CharacterSkeleton& skeleton() const;
};

inline Animation Character::animation() const {
	return _anim;
}

inline void Character::setAnimation(Animation animation) {
	_anim = animation;
}

inline const Vertices& Character::vertices() const {
	return _vertices;
}

inline const Indices& Character::indices() const {
	return _indices;
}

inline const CharacterSkeleton& Character::skeleton() const {
	return _skeleton;
}

}
