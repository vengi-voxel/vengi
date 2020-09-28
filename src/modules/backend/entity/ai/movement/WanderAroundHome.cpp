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
	const glm::vec3& target = npc.homePosition();
	const glm::vec3& pos = npc.pos();
	if (glm::all(glm::epsilonEqual(pos, target, 0.000001f))) {
		return MoveVector::Invalid;
	}
	glm::vec3 dist;
	const float homeDistance2 = glm::distance2(pos, target);
	if (homeDistance2 > _maxDistance * _maxDistance) {
		dist = target - pos;
	} else {
		dist = pos - target;
	}
	const float dot = glm::length2(dist);
	if (dot <= glm::epsilon<float>()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& dir = dist * glm::inversesqrt(dot);
	glm_assert_vec3(dir);
	math::Random random;
	const float orientation = angle(dir) + random.randomBinomial() * glm::radians(3.0f);
	return {dir * speed, orientation, true};
}

}
}
