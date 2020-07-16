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
 * @brief Checks whether the @c AI is in any or in a particular group
 *
 * If a group id is specified in the parameters, this condition only evaluates to
 * @c true if the @c AI is part of that particular group. If no parameter is
 * specified, it will evaluate to @c true if the @c AI is in any group (even if
 * the group does not contains any other member).
 */
class IsInGroup: public ICondition {
private:
	GroupId _groupId;

public:
	CONDITION_FACTORY(IsInGroup)

	explicit IsInGroup(const core::String& parameters) :
		ICondition("IsInGroup", parameters) {
		if (_parameters.empty()) {
			_groupId = -1;
		} else {
			_groupId = core::string::toInt(_parameters);
		}
	}

	virtual ~IsInGroup() {
	}

	bool evaluate(const AIPtr& entity) override {
		const GroupMgr& mgr = entity->getZone()->getGroupMgr();
		if (_groupId == -1)
			return mgr.isInAnyGroup(entity);
		return mgr.isInGroup(_groupId, entity);
	}
};

}
