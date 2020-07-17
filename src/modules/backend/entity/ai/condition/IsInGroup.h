/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/group/GroupId.h"

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

	explicit IsInGroup(const core::String& parameters);

	virtual ~IsInGroup() {
	}

	bool evaluate(const AIPtr& entity) override;
};

}
