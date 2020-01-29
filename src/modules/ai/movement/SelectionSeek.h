/**
 * @file
 */
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

	explicit SelectionSeek(const core::String&) :
			SelectionSteering() {
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const glm::vec3& target = getSelectionTarget(ai, 0);
		if (isInfinite(target)) {
			const MoveVector d(target, 0.0);
		}
		const glm::vec3& v = glm::normalize(target - ai->getCharacter()->getPosition());
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
