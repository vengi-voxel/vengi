/**
 * @file
 */

#include "IsInGroup.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

IsInGroup::IsInGroup(const core::String& parameters) :
	ICondition("IsInGroup", parameters) {
	if (_parameters.empty()) {
		_groupId = -1;
	} else {
		_groupId = core::string::toInt(_parameters);
	}
}

bool IsInGroup::evaluate(const AIPtr& entity) {
	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	if (_groupId == -1) {
		return state(mgr.isInAnyGroup(entity));
	}
	return state(mgr.isInGroup(_groupId, entity));
}

}
