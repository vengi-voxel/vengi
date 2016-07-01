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

	explicit TargetSeek(const std::string& parameters) :
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
		glm::vec3 v = _target - ai->getCharacter()->getPosition();
		double orientation = 0.0;
		if (glm::length2(v) > 0.0f) {
			orientation = angle(glm::normalize(v) * speed);
		}
		const MoveVector d(v, orientation);
		return d;
	}
};

}
}
