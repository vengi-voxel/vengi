/**
 * @file
 */

#include "WanderAroundHome.h"
#include "backend/entity/ai/common/Math.h"
#include "common/MoveVector.h"
#include "core/GLM.h"
#include "glm/gtc/epsilon.hpp"
#include "math/Random.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "core/StringUtil.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace backend {
namespace movement {

WanderAroundHome::WanderAroundHome(const core::String& parameter) :
		movement::ISteering() {
	if (parameter.empty()) {
		_maxDistance = 40.0f;
	} else {
		_maxDistance = core::string::toFloat(parameter);
	}
}

MoveVector WanderAroundHome::execute(const AIPtr& ai, float speed) const {
	backend::Npc& npc = getNpc(ai);
	const glm::vec3& target = npc.targetPosition();
	const glm::vec3& pos = npc.pos();
	if (glm::distance2(glm::vec2(target.x, target.z), glm::vec2(pos.x, pos.z)) <= 1.0f) {
		const float theta = npc.random().randomf(0.0f, glm::two_pi<float>());
		const glm::vec3& home = npc.homePosition();
		const glm::vec3 newTarget(home.x + glm::cos(theta) * _maxDistance, home.y, home.z + glm::sin(theta) * _maxDistance);
		npc.setTargetPosition(newTarget);
	}
	return seek(ai->getCharacter()->getPosition(), target, speed);
}

}
}
