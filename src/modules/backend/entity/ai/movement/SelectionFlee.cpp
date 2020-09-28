/**
 * @file
 */

#include "SelectionFlee.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

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
	return flee(ai->getCharacter()->getPosition(), target, speed);
}

}
}
