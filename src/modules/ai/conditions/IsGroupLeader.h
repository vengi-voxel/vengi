/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "common/StringUtil.h"
#include "group/GroupMgr.h"
#include "zone/Zone.h"

namespace ai {

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

	explicit IsGroupLeader(const std::string& parameters) :
		ICondition("IsGroupLeader", parameters) {
		if (_parameters.empty()) {
			_groupId = -1;
		} else {
			_groupId = std::stoi(_parameters);
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
