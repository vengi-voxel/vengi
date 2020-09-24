/**
 * @file
 */

#include "SelectionSeek.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

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
	const glm::vec3& dist = target - ai->getCharacter()->getPosition();
	if (glm::length2(dist) <= glm::epsilon<float>()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(dist);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation);
	return d;
}

}
}
