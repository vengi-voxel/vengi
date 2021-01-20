/**
 * @file
 */

#include "Wander.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include <glm/trigonometric.hpp>

namespace backend {
namespace movement {

Wander::Wander(const core::String& parameter) :
		ISteering(), _rotation(parameter.empty() ? glm::radians(10.0f) : core::string::toFloat(parameter)) {
}

static inline glm::vec3 fromRadians(float radians) {
	return glm::vec3(glm::cos(radians), 0.0f, glm::sin(radians));
}

MoveVector Wander::execute (const AIPtr& ai, float speed) const {
	const backend::ICharacterPtr& chr = ai->getCharacter();
	const float orientation = ai->getCharacter()->getOrientation();
	const glm::vec3& v = fromRadians(orientation);
	return MoveVector(v * speed, chr->random().randomBinomial() * _rotation);
}

}
}
