/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Flees the current @c IFilter selection from the given @c ICharacter
 */
class SelectionFlee: public SelectionSteering {
public:
	STEERING_FACTORY(SelectionFlee)

	explicit SelectionFlee(const core::String&) :
			SelectionSteering() {
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const glm::vec3& target = getSelectionTarget(ai, 0);
		if (isInfinite(target)) {
			const MoveVector d(target, 0.0);
		}
		const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
