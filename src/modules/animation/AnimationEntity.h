/**
 * @file
 */

#pragma once

#include "Animation.h"
#include "Skeleton.h"
#include "Vertex.h"
#include "SkeletonAttribute.h"
#include "AnimationSettings.h"
#include "AnimationCache.h"
#include "attrib/ShadowAttributes.h"
#include "math/AABB.h"
#include "core/Enum.h"
#include "core/collection/Array.h"

namespace animation {

using AnimationTimes = core::Array<float, core::enumVal(Animation::MAX) + 1>;

/**
 * Base class for animated entities that holds the vertices and indices
 * of the model
 * @ingroup Animation
 */
class AnimationEntity {
protected:
	AnimationTimes _animationTimes;
	AnimationSettings _settings;
	Vertices _vertices;
	Indices _indices;
	float _globalTimeSeconds = 0.0f;
	math::AABB<float> _aabb { -0.5f, 0.0f, -0.5f, 0.5f, 1.0f, 0.5f };

	/**
	 * @note Make sure to initialize the bones states of the skeleton before calling this
	 */
	bool updateAABB();

public:
	AnimationEntity();
	virtual ~AnimationEntity();
	void setAnimation(Animation animation, bool reset);
	void addAnimation(Animation animation, float durationSeconds);
	void removeAnimation(Animation animation);
	const AnimationTimes& animations() const;

	/**
	 * @brief Initializes the character settings with the given lua script.
	 * @note This is basically just a wrapper around initMesh() and initSettings()
	 * @return @c true if the initialization was successful, @c false otherwise.
	 */
	bool init(const AnimationCachePtr& cache, const core::String& luaString);

	virtual void shutdown() {}

	const math::AABB<float>& aabb() const;

	AnimationSettings& animationSettings();
	const AnimationSettings& animationSettings() const;

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
	virtual SkeletonAttribute& skeletonAttributes() = 0;

	virtual bool initMesh(const AnimationCachePtr& cache) = 0;
	/**
	 * @note Updating the settings without updating the mesh afterwards is pointless.
	 */
	virtual bool initSettings(const core::String& luaString) = 0;

	/**
	 * @brief Update the bone states and the tool vertices from the given inventory
	 * @param[in] dt The delta time since the last call in millis
	 * @param[in] attrib @c attrib::ShadowAttributes to get the character values
	 * from
	 */
	virtual void update(uint64_t dt, const attrib::ShadowAttributes& attrib) = 0;
};

}
