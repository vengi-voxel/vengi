#pragma once

#include "ICondition.h"
#include "common/String.h"
#include "group/GroupMgr.h"

namespace ai {

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

	IsInGroup(const std::string& parameters) :
		ICondition("IsInGroup", parameters) {
		if (_parameters.empty())
			_groupId = -1;
		else
			_groupId = std::stoi(_parameters);
	}
public:
	virtual ~IsInGroup() {
	}
	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override;
};

}
