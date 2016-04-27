#include "IsInGroup.h"
#include "AI.h"
#include "zone/Zone.h"

namespace ai {

bool IsInGroup::evaluate(const AIPtr& entity) {
	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	if (_groupId == -1)
		return mgr.isInAnyGroup(entity);
	return mgr.isInGroup(_groupId, entity);
}

CONDITION_FACTORY_IMPL(IsInGroup)

}
