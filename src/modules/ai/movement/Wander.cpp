/**
 * @file
 */

#include "Wander.h"
#include "common/Random.h"

namespace ai {
namespace movement {

Wander::Wander(const std::string& parameter) :
		ISteering(), _rotation(parameter.empty() ? ai::toRadians(10.0f) : Str::strToFloat(parameter)) {
}

MoveVector Wander::execute (const AIPtr& ai, float speed) const {
	const float orientation = ai->getCharacter()->getOrientation();
	const glm::vec3& v = fromRadians(orientation);
	const MoveVector d(v * speed, ai::randomBinomial() * _rotation);
	return d;
}

}
}
