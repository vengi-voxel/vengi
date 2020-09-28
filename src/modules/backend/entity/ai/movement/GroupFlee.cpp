/**
 * @file
 */

#include "GroupFlee.h"
#include "backend/entity/ai/AI.h"
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
		return MoveVector::Invalid;
	}
	glm::vec3 target;
	if (!zone->getGroupMgr().getPosition(_groupId, target)) {
		return MoveVector::Invalid;
	}
	return flee(ai->getCharacter()->getPosition(), target, speed);
}

}
}
