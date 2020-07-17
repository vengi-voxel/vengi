/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/group/GroupId.h"

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

	explicit IsGroupLeader(const core::String& parameters);

	virtual ~IsGroupLeader() {
	}

	bool evaluate(const AIPtr& entity) override;
};

}
