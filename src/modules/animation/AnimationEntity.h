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
#include "math/AABB.h"
#include "core/Enum.h"

namespace animation {

/**
 * Base class for animated entities that holds the vertices and indices
 * of the model
 * @ingroup Animation
 */
class AnimationEntity {
protected:
	Animation _anim = Animation::IDLE;
	AnimationSettings _settings;
	Vertices _vertices;
	Indices _indices;
	float _globalTimeSeconds = 0.0f;
	math::AABB<float> _aabb { -0.5f, 0.0f, -0.5f, 0.5f, 1.0f, 0.5f };

	/**
	 * @note Make sure to initialize the bones states of the skeleton before calling this
	 */
	bool updateAABB() {
		glm::mat4 boneMatrices[std::enum_value(BoneId::Max)];
		for (int i = 0; i < std::enum_value(BoneId::Max); ++i) {
			boneMatrices[i] = skeleton().bone((BoneId)i).matrix();
		}
		_aabb.setLowerCorner(glm::zero<glm::vec3>());
		_aabb.setUpperCorner(glm::zero<glm::vec3>());
		for (const auto& v : _vertices) {
			const glm::vec4& p = boneMatrices[v.boneId] * glm::vec4(v.pos, 1.0f);
			_aabb.accumulate(p.x, p.y, p.z);
		}
		return _aabb.isValid();
	}

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
		if (!initMesh(cache)) {
			return false;
		}
		return updateAABB();
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
	virtual SkeletonAttribute& skeletonAttributes() = 0;

	const math::AABB<float>& aabb() const;

	AnimationSettings& animationSettings();
	const AnimationSettings& animationSettings() const;
};

inline const math::AABB<float>& AnimationEntity::aabb() const {
	return _aabb;
}

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
