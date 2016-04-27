#include "IsCloseToGroup.h"
#include "ICharacter.h"
#include "AI.h"
#include "zone/Zone.h"

namespace ai {

bool IsCloseToGroup::evaluate(const AIPtr& entity) {
	if (_groupId == -1) {
		return false;
	}

	if (_distance < 0.0f) {
		return false;
	}

	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	const glm::vec3& pos = mgr.getPosition(_groupId);
	if (isInfinite(pos)) {
		return false;
	}
	return glm::distance(pos, entity->getCharacter()->getPosition()) <= _distance;
}

CONDITION_FACTORY_IMPL(IsCloseToGroup)

}
