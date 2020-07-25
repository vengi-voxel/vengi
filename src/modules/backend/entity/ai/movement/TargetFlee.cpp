/**
 * @file
 */

#include "TargetFlee.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include <glm/geometric.hpp>

namespace backend {
namespace movement {

TargetFlee::TargetFlee(const core::String& parameters) :
		ISteering() {
	_valid = parse(parameters, _target);
}

bool TargetFlee::isValid () const {
	return _valid;
}

MoveVector TargetFlee::execute (const AIPtr& ai, float speed) const {
	if (!isValid()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - _target);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation, true);
	return d;
}

}
}
