/**
 * @file
 */

#include "GroupFlee.h"

namespace ai {
namespace movement {

GroupFlee::GroupFlee(const std::string& parameters) :
		ISteering() {
	_groupId = ::atoi(parameters.c_str());
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
