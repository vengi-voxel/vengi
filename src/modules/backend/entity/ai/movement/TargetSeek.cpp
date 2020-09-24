/**
 * @file
 */

#include "TargetSeek.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

namespace backend {
namespace movement {

TargetSeek::TargetSeek(const core::String& parameters) :
		ISteering() {
	_valid = parse(parameters, _target);
}

bool TargetSeek::isValid () const {
	return _valid;
}

MoveVector TargetSeek::execute (const AIPtr& ai, float speed) const {
	if (!isValid()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& dist = _target - ai->getCharacter()->getPosition();
	if (glm::length2(dist) <= glm::epsilon<float>()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(dist);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation, true);
	return d;
}

}
}
