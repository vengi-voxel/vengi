/**
 * @file
 */

#pragma once

#include "Animation.h"
#include "Skeleton.h"
#include "Vertex.h"
#include "SkeletonAttribute.h"
#include "AnimationSettings.h"
#include "animation/AnimationCache.h"
#include "attrib/ShadowAttributes.h"

namespace animation {

/**
 * Base class for animated entities that holds the vertices and indices
 * of the model
 * @ingroup Animation
 */
class AnimationEntity {
protected:
	Animation _anim = Animation::Idle;
	AnimationSettings _settings;
	Vertices _vertices;
	Indices _indices;
	float _globalTimeSeconds = 0.0f;
public:
	virtual ~AnimationEntity() {}
	void setAnimation(Animation animation);
	Animation animation() const;

	/**
	 * @brief Initializes the character settings with the given lua script.
	 * @note This is basically just a wrapper around initMesh() and initSettings()
	 * @return @c true if the initialization was successful, @c false otherwise.
	 */
	bool init(const AnimationCachePtr& cache, const std::string& luaString) {
		if (!initSettings(luaString)) {
			return false;
		}
		return initMesh(cache);
	}
	virtual void shutdown() {}
	virtual bool initMesh(const AnimationCachePtr& cache) = 0;
	/**
	 * @note Updating the settings without updating the mesh afterwards is pointless.
	 */
	virtual bool initSettings(const std::string& luaString) = 0;

	/**
	 * @brief Update the bone states and the tool vertices from the given inventory
	 * @param[in] dt The delta time since the last call
	 * @param[in] attrib @c attrib::ShadowAttributes to get the character values
	 * from
	 */
	virtual void update(uint64_t dt, const attrib::ShadowAttributes& attrib) = 0;

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
	virtual const Skeleton& skeleton() const = 0;
	virtual SkeletonAttribute& skeletonAttributes() = 0;;

	AnimationSettings& animationSettings();
	const AnimationSettings& animationSettings() const;
};

inline Animation AnimationEntity::animation() const {
	return _anim;
}

inline void AnimationEntity::setAnimation(Animation animation) {
	_anim = animation;
}

inline const Vertices& AnimationEntity::vertices() const {
	return _vertices;
}

inline const Indices& AnimationEntity::indices() const {
	return _indices;
}

inline AnimationSettings& AnimationEntity::animationSettings() {
	return _settings;
}

inline const AnimationSettings& AnimationEntity::animationSettings() const {
	return _settings;
}

}
