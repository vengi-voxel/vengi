/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/common/StringUtil.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

/**
 * @brief Evaluates to true if you are the first member in a particular group
 *
 * The parameter that is expected is the group id
 */
class IsGroupLeader: public ICondition {
private:
	GroupId _groupId;
public:
	CONDITION_FACTORY(IsGroupLeader)

	explicit IsGroupLeader(const core::String& parameters) :
		ICondition("IsGroupLeader", parameters) {
		if (_parameters.empty()) {
			_groupId = -1;
		} else {
			_groupId = core::string::toInt(_parameters);
		}
	}

	virtual ~IsGroupLeader() {
	}

	bool evaluate(const AIPtr& entity) override {
		if (_groupId == -1) {
			return false;
		}
		const GroupMgr& mgr = entity->getZone()->getGroupMgr();
		return mgr.isGroupLeader(_groupId, entity);
	}
};

}
