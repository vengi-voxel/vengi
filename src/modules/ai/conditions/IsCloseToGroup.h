#pragma once

#include "ICondition.h"
#include "common/String.h"
#include "group/GroupMgr.h"

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

	IsCloseToGroup(const std::string& parameters) :
		ICondition("IsCloseToGroup", parameters) {
		std::vector<std::string> tokens;
		Str::splitString(_parameters, tokens, ",");
		if (tokens.size() != 2) {
			_groupId = -1;
			_distance = -1.0f;
		} else {
			_groupId = std::stoi(tokens[0]);
			_distance = std::stof(tokens[1]);
		}
	}
public:
	virtual ~IsCloseToGroup() {
	}
	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override;
};

}
