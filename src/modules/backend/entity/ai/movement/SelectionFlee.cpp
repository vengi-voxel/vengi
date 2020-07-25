/**
 * @file
 */

#include "SelectionFlee.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include <glm/geometric.hpp>

namespace backend {
namespace movement {

SelectionFlee::SelectionFlee(const core::String&) :
		SelectionSteering() {
}

MoveVector SelectionFlee::execute(const AIPtr& ai, float speed) const {
	glm::vec3 target;
	if (!getSelectionTarget(ai, 0, target)) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation);
	return d;
}

}
}
