/**
 * @file
 */

#include "GroupSeek.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {
namespace movement {

GroupSeek::GroupSeek(const core::String& parameters) :
		ISteering() {
	_groupId = parameters.toInt();
}

MoveVector GroupSeek::execute (const AIPtr& ai, float speed) const {
	const Zone* zone = ai->getZone();
	if (zone == nullptr) {
		return MoveVector(glm::vec3(), 0.0f, false);
	}
	glm::vec3 target;
	if (!zone->getGroupMgr().getPosition(_groupId, target)) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = glm::normalize(target - ai->getCharacter()->getPosition());
	const float orientation = angle(v);
	const MoveVector d(v * speed, orientation, true);
	return d;
}

}
}
