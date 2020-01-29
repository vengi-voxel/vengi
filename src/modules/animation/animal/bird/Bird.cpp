/**
 * @file
 */

#include "Bird.h"
#include "animation/Animation.h"
#include "BirdSkeletonAttribute.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "anim/Idle.h"
#include "anim/Run.h"

namespace animation {

bool Bird::initSettings(const core::String& luaString) {
	AnimationSettings settings;
	BirdSkeletonAttribute attributes;
	if (loadAnimationSettings(luaString, settings, &attributes)) {
		if (!attributes.init()) {
			return false;
		}
		_settings = settings;
		_attributes = attributes;
		return true;
	}
	Log::warn("Failed to load the Bird settings");
	return false;
}

bool Bird::initMesh(const AnimationCachePtr& cache) {
	if (!cache->getBoneModel(_settings, _vertices, _indices)) {
		Log::warn("Failed to load the models");
		return false;
	}
	return true;
}

void Bird::update(uint64_t dt, const attrib::ShadowAttributes& attrib) {
	const float animTimeSeconds = float(dt) / 1000.0f;

	const BirdSkeleton old = _skeleton;
	const float velocity = (float)attrib.current(attrib::Type::SPEED);

	switch (_anim) {
	case Animation::IDLE:
		animal::bird::idle::update(_globalTimeSeconds, _skeleton, _attributes);
		break;
	case Animation::RUN:
		animal::bird::run::update(_globalTimeSeconds, velocity, _skeleton, _attributes);
		break;
	default:
		break;
	}

	if (_globalTimeSeconds > 0.0f) {
		const float ticksPerSecond = 15.0f;
		_skeleton.lerp(old, glm::min(1.0f, animTimeSeconds * ticksPerSecond));
	}

	_globalTimeSeconds += animTimeSeconds;
}

}
