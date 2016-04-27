#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Flees from a particular target
 */
class TargetFlee: public ISteering {
protected:
	glm::vec3 _target;
public:
	STEERING_FACTORY

	TargetFlee(const std::string& parameters) :
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
		glm::vec3 v = ai->getCharacter()->getPosition();
		v -= _target;
		double orientation = 0.0;
		if (glm::length2(v) > 0.0f) {
			orientation = angle(glm::normalize(v) * speed);
		}

		const MoveVector d(v, orientation);
		return d;
	}

	std::ostream& print(std::ostream& stream, int level) const override {
		for (int i = 0; i < level; ++i) {
			stream << '\t';
		}
		stream << "TargetFlee(" << _target.x << "," << _target.y << "," << _target.z << ")";
		return stream;
	}
};


}
}
