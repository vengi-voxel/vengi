/**
 * @file
 */

#include "IsCloseToGroup.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"
#include "core/collection/DynamicArray.h"
#include <glm/geometric.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace backend {

IsCloseToGroup::IsCloseToGroup(const core::String& parameters) :
	ICondition("IsCloseToGroup", parameters) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(_parameters, tokens, ",");
	if (tokens.size() != 2u) {
		_groupId = -1;
		_distance = -1.0f;
	} else {
		_groupId = core::string::toInt(tokens[0]);
		_distance = core::string::toFloat(tokens[1]);
	}
	_distance = glm::pow(_distance, 2.0);
}

bool IsCloseToGroup::evaluate(const AIPtr& entity) {
	if (_groupId == -1) {
		return state(false);
	}

	if (_distance < 0.0f) {
		return state(false);
	}

	const GroupMgr& mgr = entity->getZone()->getGroupMgr();
	glm::vec3 pos;
	if (!mgr.getPosition(_groupId, pos)) {
		return state(false);
	}
	return state(glm::distance2(pos, entity->getCharacter()->getPosition()) <= _distance);
}

}
