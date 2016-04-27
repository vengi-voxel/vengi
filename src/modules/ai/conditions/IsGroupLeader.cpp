#include "IsGroupLeader.h"
#include "AI.h"
#include "zone/Zone.h"

namespace ai {

bool IsGroupLeader::evaluate(const AIPtr& entity) {
	if (_groupId == -1)
		return false;
	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	return mgr.isGroupLeader(_groupId, entity);
}

CONDITION_FACTORY_IMPL(IsGroupLeader)

}
