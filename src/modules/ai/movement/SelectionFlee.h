#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Flees the current @c IFilter selection from the given @c ICharacter
 */
class SelectionFlee: public SelectionSteering {
public:
	STEERING_FACTORY

	SelectionFlee(const std::string&) :
			SelectionSteering() {
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const glm::vec3& target = getSelectionTarget(ai, 0);
		if (isInfinite(target)) {
			const MoveVector d(target, 0.0);
		}
		glm::vec3 v = ai->getCharacter()->getPosition() - target;
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
		stream << "SelectionFlee()";
		return stream;
	}
};

}
}
