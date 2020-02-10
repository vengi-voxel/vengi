/**
 * @file
 */

#include "Wander.h"
#include "common/Random.h"
#include "core/StringUtil.h"

namespace ai {
namespace movement {

Wander::Wander(const core::String& parameter) :
		ISteering(), _rotation(parameter.empty() ? ai::toRadians(10.0f) : core::string::toFloat(parameter)) {
}

MoveVector Wander::execute (const AIPtr& ai, float speed) const {
	const float orientation = ai->getCharacter()->getOrientation();
	const glm::vec3& v = fromRadians(orientation);
	const MoveVector d(v * speed, ai::randomBinomial() * _rotation);
	return d;
}

}
}
