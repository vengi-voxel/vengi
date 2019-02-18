/**
 * @file
 */

#pragma once

#include "animation/tb_animation.h"

namespace tb {

// TBAnimatedFloat - A animated float value

class TBAnimatedFloat : public TBAnimationObject {
public:
	float src_val;
	float dst_val;
	float current_progress;

public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBAnimatedFloat, TBAnimationObject);

	TBAnimatedFloat(float initial_value, ANIMATION_CURVE animation_curve = ANIMATION_DEFAULT_CURVE,
					double animation_duration = ANIMATION_DEFAULT_DURATION)
		: src_val(initial_value), dst_val(initial_value), current_progress(0) {
		TBAnimationObject::animation_curve = animation_curve;
		TBAnimationObject::animation_duration = animation_duration;
	}

	float getValue() const {
		return src_val + (dst_val - src_val) * current_progress;
	}
	void setValueAnimated(float value) {
		src_val = getValue();
		dst_val = value;
		TBAnimationManager::startAnimation(this, animation_curve, animation_duration);
	}
	void setValueImmediately(float value) {
		TBAnimationManager::abortAnimation(this, false);
		src_val = dst_val = value;
		onAnimationUpdate(1.0f);
	}

	virtual void onAnimationStart() {
		current_progress = 0;
	}
	virtual void onAnimationUpdate(float progress) {
		current_progress = progress;
	}
	virtual void onAnimationStop(bool aborted) {
	}
};

// TBFloatAnimator - Animates a external float value, which address is given in the constructor.

class TBFloatAnimator : public TBAnimatedFloat {
public:
	float *target_value;

public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBFloatAnimator, TBAnimationObject);

	TBFloatAnimator(float *target_value, ANIMATION_CURVE animation_curve = ANIMATION_DEFAULT_CURVE,
					double animation_duration = ANIMATION_DEFAULT_DURATION)
		: TBAnimatedFloat(*target_value), target_value(target_value) {
	}

	virtual void onAnimationStart() {
		TBAnimatedFloat::onAnimationStart();
		*target_value = getValue();
	}
	virtual void onAnimationUpdate(float progress) {
		TBAnimatedFloat::onAnimationUpdate(progress);
		*target_value = getValue();
	}
};

} // namespace tb
