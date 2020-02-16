/**
 * @file
 */

#include "AnimationEntity.h"

namespace animation {

bool AnimationEntity::updateAABB() {
	glm::mat4 bones[shader::SkeletonShaderConstants::getMaxBones()] {};
	skeleton().update(_settings, bones);
	_aabb.setLowerCorner(glm::zero<glm::vec3>());
	_aabb.setUpperCorner(glm::zero<glm::vec3>());
	for (const auto& v : _vertices) {
		const glm::vec4& p = bones[v.boneId] * glm::vec4(v.pos, 1.0f);
		_aabb.accumulate(p.x, p.y, p.z);
	}
	return _aabb.isValid();
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
	return updateAABB();
}

const math::AABB<float>& AnimationEntity::aabb() const {
	return _aabb;
}

Animation AnimationEntity::animation() const {
	return _anim;
}

void AnimationEntity::setAnimation(Animation animation) {
	_anim = animation;
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
