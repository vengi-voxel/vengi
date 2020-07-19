/**
 * @file
 */

#include "TargetSeek.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

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
	const glm::vec3& v = glm::normalize(_target - ai->getCharacter()->getPosition());
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation, true);
	return d;
}

}
}
