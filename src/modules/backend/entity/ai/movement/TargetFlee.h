/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

/**
 * @brief Flees from a particular target
 */
class TargetFlee: public ISteering {
protected:
	glm::vec3 _target;
public:
	STEERING_FACTORY(TargetFlee)

	explicit TargetFlee(const core::String& parameters) :
			ISteering() {
		_target = parse(parameters);
	}

	inline bool isValid () const {
		return !isInfinite(_target);
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		if (!isValid()) {
			return MoveVector(_target, 0.0f);
		}
		const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - _target);
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};


}
}
