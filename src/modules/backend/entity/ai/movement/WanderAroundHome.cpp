/**
 * @file
 */

#include "WanderAroundHome.h"
#include "backend/entity/Npc.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace backend {

ai::MoveVector WanderAroundHome::execute(const ai::AIPtr& ai, float speed) const {
	backend::Npc& npc = getNpc(ai);
	const glm::vec3 target(npc.homePosition());
	const glm::vec3& pos = npc.pos();
	glm::vec3 v;
	if (glm::distance2(pos, target) > _maxDistance) {
		v = glm::normalize(target - pos);
	} else {
		v = glm::normalize(pos - target);
	}
	const float orientation = ai::angle(v) + ai::randomBinomial() * ai::toRadians(3.0f);
	const ai::MoveVector d(v * speed, orientation);
	return d;
}

}
