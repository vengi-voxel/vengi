/**
 * @file
 */

#include "GroupFlee.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/zone/Zone.h"
#include <glm/geometric.hpp>

namespace backend {
namespace movement {

GroupFlee::GroupFlee(const core::String& parameters) :
		ISteering() {
	_groupId = parameters.toInt();
}

MoveVector GroupFlee::execute (const AIPtr& ai, float speed) const {
	const Zone* zone = ai->getZone();
	if (zone == nullptr) {
		return MoveVector::Invalid;
	}
	glm::vec3 target;
	if (!zone->getGroupMgr().getPosition(_groupId, target)) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation, true);
	return d;
}

}
}
