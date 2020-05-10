/**
 * @file
 */

#include "AnimationEntity.h"
#include <float.h>

namespace animation {

bool AnimationEntity::updateAABB() {
	glm::mat4 bones[shader::SkeletonShaderConstants::getMaxBones()] {};
	skeleton().update(_settings, bones);
	_aabb.setLowerCorner(glm::vec3(0.0f));
	_aabb.setUpperCorner(glm::vec3(0.0f));
	for (const auto& v : _vertices) {
		const glm::vec4& p = bones[v.boneId] * glm::vec4(v.pos, 1.0f);
		_aabb.accumulate(p.x, p.y, p.z);
	}
	return _aabb.isValid();
}

AnimationEntity::AnimationEntity() {
	_animationTimes.fill(0.0);
}

AnimationEntity::~AnimationEntity() {
}

bool AnimationEntity::init(const AnimationCachePtr& cache, const core::String& luaString) {
	if (!initSettings(luaString)) {
		return false;
	}
	if (!initMesh(cache)) {
		return false;
	}
	setAnimation(animation::Animation::IDLE, false);
	return updateAABB();
}

const math::AABB<float>& AnimationEntity::aabb() const {
	return _aabb;
}

const AnimationTimes& AnimationEntity::animations() const {
	return _animationTimes;
}

void AnimationEntity::addAnimation(Animation animation, double durationSeconds) {
	_animationTimes[core::enumVal(animation)] = _globalTimeSeconds + durationSeconds;
}

void AnimationEntity::removeAnimation(Animation animation) {
	_animationTimes[core::enumVal(animation)] = 0.0;
}

void AnimationEntity::setAnimation(Animation animation, bool reset) {
	if (reset) {
		_animationTimes.fill(0.0);
	}
	_animationTimes[core::enumVal(animation)] = FLT_MAX;
}

const Vertices& AnimationEntity::vertices() const {
	return _vertices;
}

const Indices& AnimationEntity::indices() const {
	return _indices;
}

AnimationSettings& AnimationEntity::animationSettings() {
	return _settings;
}

const AnimationSettings& AnimationEntity::animationSettings() const {
	return _settings;
}

}
