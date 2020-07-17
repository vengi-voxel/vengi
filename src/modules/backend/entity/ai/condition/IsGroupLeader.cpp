/**
 * @file
 */

#include "IsGroupLeader.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"
#include "core/StringUtil.h"

namespace backend {

IsGroupLeader::IsGroupLeader(const core::String& parameters) :
	ICondition("IsGroupLeader", parameters) {
	if (_parameters.empty()) {
		_groupId = -1;
	} else {
		_groupId = core::string::toInt(_parameters);
	}
}

bool IsGroupLeader::evaluate(const AIPtr& entity) {
	if (_groupId == -1) {
		return false;
	}
	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	return mgr.isGroupLeader(_groupId, entity);
}

}
