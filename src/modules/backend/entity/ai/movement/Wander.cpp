/**
 * @file
 */

#include "Wander.h"
#include "math/Random.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

namespace backend {
namespace movement {

Wander::Wander(const core::String& parameter) :
		ISteering(), _rotation(parameter.empty() ? toRadians(10.0f) : core::string::toFloat(parameter)) {
}

MoveVector Wander::execute (const AIPtr& ai, float speed) const {
	const float orientation = ai->getCharacter()->getOrientation();
	const glm::vec3& v = fromRadians(orientation);
	math::Random random;
	const MoveVector d(v * speed, random.randomBinomial() * _rotation);
	return d;
}

}
}
