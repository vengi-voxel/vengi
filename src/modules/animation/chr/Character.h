/**
 * @file
 */

#pragma once

#include "animation/AnimationEntity.h"
#include "CharacterSkeleton.h"
#include "stock/Stock.h"
#include <stdint.h>

namespace animation {

/**
 * @brief Handles the loading and creation of the meshes and the bones regarding
 * the given @c CharacterSettings
 * @ingroup Animation
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
	void shutdown() override;
	bool initMesh(const AnimationCachePtr& cache) override;
	bool initSettings(const core::String& luaString) override;
	void update(double deltaSeconds, const attrib::ShadowAttributes& attrib) override;
	/**
	 * @brief Updates the vertices and indices buffer whenever the character switched the active tool
	 * @param[in] cache The cache that is used to resolve the item models
	 * @param[in] stock The stock::Stock object to query the active items
	 */
	bool updateTool(const AnimationCachePtr& cache, const stock::Stock& stock);
	const Skeleton& skeleton() const override;
	SkeletonAttribute& skeletonAttributes() override;
	const CharacterSkeletonAttribute& skeletonAttributes() const;
};

inline SkeletonAttribute& Character::skeletonAttributes() {
	return _attributes;
}

inline const CharacterSkeletonAttribute& Character::skeletonAttributes() const {
	return _attributes;
}

}
