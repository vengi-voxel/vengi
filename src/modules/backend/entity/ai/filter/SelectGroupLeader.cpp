/**
 * @file
 */

#include "SelectGroupLeader.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/AI.h"
#include "core/StringUtil.h"
#include "backend/entity/ai/filter/FilteredEntities.h"

namespace backend {

SelectGroupLeader::SelectGroupLeader(const core::String& parameters) :
	IFilter("SelectGroupLeader", parameters) {
	if (_parameters.empty()) {
		_groupId = -1;
	} else {
		_groupId = core::string::toInt(_parameters);
	}
}

void SelectGroupLeader::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	const Zone* zone = entity->getZone();
	const GroupMgr& groupMgr = zone->getGroupMgr();
	const AIPtr& groupLeader = groupMgr.getLeader(_groupId);
	if (groupLeader) {
		entities.push_back(groupLeader->getId());
	}
}

}
