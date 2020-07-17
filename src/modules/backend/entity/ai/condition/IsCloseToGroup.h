/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/group/GroupId.h"

namespace backend {

/**
 * @brief Checks whether the controlled @c AI is close to a particular group.
 *
 * The parameters are given as the group id and the distance to the group that
 * triggers this condition to evaluate to @c true.
 */
class IsCloseToGroup: public ICondition {
private:
	GroupId _groupId;
	float _distance;
public:
	CONDITION_FACTORY(IsCloseToGroup)

	explicit IsCloseToGroup(const core::String& parameters);

	virtual ~IsCloseToGroup() {
	}

	bool evaluate(const AIPtr& entity) override;
};

}
