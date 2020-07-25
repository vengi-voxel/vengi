/**
 * @file
 */

#include "IsCloseToGroup.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"
#include <glm/geometric.hpp>
#include <vector>

namespace backend {

IsCloseToGroup::IsCloseToGroup(const core::String& parameters) :
	ICondition("IsCloseToGroup", parameters) {
	std::vector<core::String> tokens;
	core::string::splitString(_parameters, tokens, ",");
	if (tokens.size() != 2) {
		_groupId = -1;
		_distance = -1.0f;
	} else {
		_groupId = core::string::toInt(tokens[0]);
		_distance = core::string::toFloat(tokens[1]);
	}
}

bool IsCloseToGroup::evaluate(const AIPtr& entity) {
	if (_groupId == -1) {
		return false;
	}

	if (_distance < 0.0f) {
		return false;
	}

	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	glm::vec3 pos;
	if (!mgr.getPosition(_groupId, pos)) {
		return false;
	}
	return glm::distance(pos, entity->getCharacter()->getPosition()) <= _distance;
}

}
