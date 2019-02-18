/**
 * @file
 */

#include "animation/tb_animation.h"
#include "core/Assert.h"
#include "tb_system.h"

namespace tb {

#define SMOOTHSTEP(x) ((x) * (x) * (3.0f - 2.0f * (x)))

static float sc(float x) {
	float s = x < 0 ? -1.f : 1.f;
	x = Abs(x);
	if (x >= 1)
		return s;
	return s * (x < 0 ? x / 0.5f : (x / (1 + x * x)) / 0.5f);
}

static float smoothCurve(float x, float a) {
	float r = a * x / (2 * a * x - a - x + 1);
	r = (r - 0.5f) * 2;
	return sc(r) * 0.5f + 0.5f;
}

void TBAnimationObject::invokeOnAnimationStart() {
	TBLinkListOf<TBAnimationListener>::Iterator li = m_listeners.iterateForward();
	onAnimationStart();
	while (TBAnimationListener *listener = li.getAndStep())
		listener->onAnimationStart(this);
}

void TBAnimationObject::invokeOnAnimationUpdate(float progress) {
	TBLinkListOf<TBAnimationListener>::Iterator li = m_listeners.iterateForward();
	onAnimationUpdate(progress);
	while (TBAnimationListener *listener = li.getAndStep())
		listener->onAnimationUpdate(this, progress);
}

void TBAnimationObject::invokeOnAnimationStop(bool aborted) {
	TBLinkListOf<TBAnimationListener>::Iterator li = m_listeners.iterateForward();
	onAnimationStop(aborted);
	while (TBAnimationListener *listener = li.getAndStep())
		listener->onAnimationStop(this, aborted);
}

TBLinkListOf<TBAnimationObject> TBAnimationManager::animating_objects;
static int block_animations_counter = 0;

// static
void TBAnimationManager::abortAllAnimations() {
	while (TBAnimationObject *obj = animating_objects.getFirst())
		abortAnimation(obj, true);
}

// static
void TBAnimationManager::update() {
	double time_now = TBSystem::getTimeMS();

	TBLinkListOf<TBAnimationObject>::Iterator iter = animating_objects.iterateForward();
	while (TBAnimationObject *obj = iter.getAndStep()) {
		// Adjust the start time if it's the first update time for this object.
		if (obj->adjust_start_time) {
			obj->animation_start_time = time_now;
			obj->adjust_start_time = false;
		}

		// Calculate current progress
		// If animation_duration is 0, it should just complete immediately.
		float progress = 1.0f;
		if (obj->animation_duration != 0) {
			progress = (float)(time_now - obj->animation_start_time) / (float)obj->animation_duration;
			progress = Min(progress, 1.0f);
		}

		// Apply animation curve
		float tmp;
		switch (obj->animation_curve) {
		case ANIMATION_CURVE_SLOW_DOWN:
			tmp = 1 - progress;
			progress = 1 - tmp * tmp * tmp;
			break;
		case ANIMATION_CURVE_SPEED_UP:
			progress = progress * progress * progress;
			break;
		case ANIMATION_CURVE_BEZIER:
			progress = SMOOTHSTEP(progress);
			break;
		case ANIMATION_CURVE_SMOOTH:
			progress = smoothCurve(progress, 0.6f);
			break;
		default: // linear (progress is already linear)
			break;
		}

		// Update animation
		obj->invokeOnAnimationUpdate(progress);

		// Remove completed animations
		if (progress == 1.0f) {
			animating_objects.remove(obj);
			obj->invokeOnAnimationStop(false);
			delete obj;
		}
	}
}

// static
bool TBAnimationManager::hasAnimationsRunning() {
	return animating_objects.hasLinks();
}

// static
void TBAnimationManager::startAnimation(TBAnimationObject *obj, ANIMATION_CURVE animationCurve,
										double animationDuration, ANIMATION_TIME animationTime) {
	if (obj->isAnimating())
		abortAnimation(obj, false);
	if (isAnimationsBlocked())
		animationDuration = 0;
	obj->adjust_start_time = (animationTime == ANIMATION_TIME_FIRST_UPDATE ? true : false);
	obj->animation_start_time = TBSystem::getTimeMS();
	obj->animation_duration = Max(animationDuration, 0.0);
	obj->animation_curve = animationCurve;
	animating_objects.addLast(obj);
	obj->invokeOnAnimationStart();
}

// static
void TBAnimationManager::abortAnimation(TBAnimationObject *obj, bool deleteAnimation) {
	if (obj->isAnimating()) {
		animating_objects.remove(obj);
		obj->invokeOnAnimationStop(true);
		if (deleteAnimation)
			delete obj;
	}
}

// static
bool TBAnimationManager::isAnimationsBlocked() {
	return block_animations_counter > 0;
}

// static
void TBAnimationManager::beginBlockAnimations() {
	block_animations_counter++;
}

// static
void TBAnimationManager::endBlockAnimations() {
	core_assert(block_animations_counter > 0);
	block_animations_counter--;
}

} // namespace tb
