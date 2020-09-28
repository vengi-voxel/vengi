/**
 * @file
 */

#include "TargetFlee.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

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
	return flee(ai->getCharacter()->getPosition(), _target, speed);

}

}
}
