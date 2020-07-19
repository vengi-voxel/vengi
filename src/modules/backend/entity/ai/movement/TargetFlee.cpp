/**
 * @file
 */

#include "TargetFlee.h"

namespace backend {
namespace movement {

TargetFlee::TargetFlee(const core::String& parameters) :
		ISteering() {
	_target = parse(parameters);
}

bool TargetFlee::isValid () const {
	return !isInfinite(_target);
}

MoveVector TargetFlee::execute (const AIPtr& ai, float speed) const {
	if (!isValid()) {
		return MoveVector(_target, 0.0f);
	}
	const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - _target);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation);
	return d;
}

}
}
