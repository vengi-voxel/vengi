/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Seeks a particular target
 */
class TargetSeek: public ISteering {
protected:
	glm::vec3 _target;
public:
	STEERING_FACTORY(TargetSeek)

	explicit TargetSeek(const core::String& parameters) :
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
		const glm::vec3& v = glm::normalize(_target - ai->getCharacter()->getPosition());
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
