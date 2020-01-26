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

	explicit IsCloseToGroup(const core::String& parameters) :
		ICondition("IsCloseToGroup", parameters) {
		std::vector<core::String> tokens;
		Str::splitString(_parameters, tokens, ",");
		if (tokens.size() != 2) {
			_groupId = -1;
			_distance = -1.0f;
		} else {
			_groupId = core::string::toInt(tokens[0]);
			_distance = core::string::toFloat(tokens[1]);
		}
	}

	virtual ~IsCloseToGroup() {
	}

	bool evaluate(const AIPtr& entity) override {
		if (_groupId == -1) {
			return false;
		}

		if (_distance < 0.0f) {
			return false;
		}

		const GroupMgr& mgr = entity->getZone()->getGroupMgr();
		const glm::vec3& pos = mgr.getPosition(_groupId);
		if (isInfinite(pos)) {
			return false;
		}
		return glm::distance(pos, entity->getCharacter()->getPosition()) <= _distance;
	}
};

}
