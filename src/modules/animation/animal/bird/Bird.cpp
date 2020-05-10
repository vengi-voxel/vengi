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
#include "core/Log.h"

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

void Bird::update(double deltaSeconds, const attrib::ShadowAttributes& attrib) {
	const BirdSkeleton old = _skeleton;
	const double velocity = attrib.current(attrib::Type::SPEED);

	// TODO: lerp the animations
	for (int i = 0; i <= core::enumVal(Animation::MAX); ++i) {
		const double aTime = _animationTimes[i];
		if (aTime < _globalTimeSeconds) {
			continue;
		}
		const Animation anim = (Animation)i;
		switch (anim) {
		case Animation::IDLE:
			animal::bird::idle::update(_globalTimeSeconds, _skeleton, _attributes);
			break;
		case Animation::RUN:
			animal::bird::run::update(_globalTimeSeconds, velocity, _skeleton, _attributes);
			break;
		default:
			break;
		}
	}

	if (_globalTimeSeconds > 0.0) {
		_skeleton.lerp(old, deltaSeconds);
	}

	_globalTimeSeconds += deltaSeconds;
}

}
