/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
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
		glm::vec3 target;
		if (!getSelectionTarget(ai, 0, target)) {
			return MoveVector::Invalid;
		}
		const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
