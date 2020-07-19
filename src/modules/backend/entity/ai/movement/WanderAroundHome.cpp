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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace backend {
namespace movement {

MoveVector WanderAroundHome::execute(const AIPtr& ai, float speed) const {
	backend::Npc& npc = getNpc(ai);
	const glm::vec3& target = npc.homePosition();
	const glm::vec3& pos = npc.pos();
	if (glm::all(glm::epsilonEqual(pos, target, 0.000001f))) {
		return MoveVector::Invalid;
	}
	glm::vec3 v;
	const float homeDistance2 = glm::distance2(pos, target);
	if (homeDistance2 > _maxDistance * _maxDistance) {
		v = target - pos;
	} else {
		v = pos - target;
	}
	const glm::vec3 dir = glm::normalize(v);
	glm_assert_vec3(dir);
	math::Random random;
	const float orientation = angle(dir) + random.randomBinomial() * glm::radians(3.0f);
	const MoveVector d(dir * speed, orientation, true);
	return d;
}

}
}
