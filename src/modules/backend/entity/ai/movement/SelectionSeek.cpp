/**
 * @file
 */

#include "SelectionSeek.h"
#include "backend/entity/ai/AI.h"

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
	const glm::vec3& v = glm::normalize(target - ai->getCharacter()->getPosition());
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation);
	return d;
}

}
}
