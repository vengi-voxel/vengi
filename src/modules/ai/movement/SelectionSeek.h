#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Seeks the current @c IFilter selection from the given @c ICharacter
 */
class SelectionSeek: public SelectionSteering {
public:
	STEERING_FACTORY(SelectionSeek)

	explicit SelectionSeek(const std::string&) :
			SelectionSteering() {
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const glm::vec3& target = getSelectionTarget(ai, 0);
		if (isInfinite(target)) {
			const MoveVector d(target, 0.0);
		}
		glm::vec3 v = target - ai->getCharacter()->getPosition();
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
