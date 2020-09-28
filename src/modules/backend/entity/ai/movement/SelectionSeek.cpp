/**
 * @file
 */

#include "SelectionSeek.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

namespace backend {
namespace movement {

SelectionSeek::SelectionSeek(const core::String&) :
		SelectionSteering() {
}

MoveVector SelectionSeek::execute(const AIPtr& ai, float speed) const {
	glm::vec3 target;
	if (!getSelectionTarget(ai, 0, target)) {
		return MoveVector::Invalid;
	}
	return seek(ai->getCharacter()->getPosition(), target, speed);
}

}
}
