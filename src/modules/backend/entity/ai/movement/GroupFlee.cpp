/**
 * @file
 */

#include "GroupFlee.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {
namespace movement {

GroupFlee::GroupFlee(const core::String& parameters) :
		ISteering() {
	_groupId = parameters.toInt();
}

MoveVector GroupFlee::execute (const AIPtr& ai, float speed) const {
	const Zone* zone = ai->getZone();
	if (zone == nullptr) {
		return MoveVector(VEC3_INFINITE, 0.0f);
	}
	const glm::vec3& target = zone->getGroupMgr().getPosition(_groupId);
	if (isInfinite(target)) {
		return MoveVector(target, 0.0f);
	}
	const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation);
	return d;
}

}
}
