/**
 * @file
 */

#include "SelectGroupMembers.h"
#include "backend/entity/ai/zone/Zone.h"
#include "core/StringUtil.h"

namespace backend {

SelectGroupMembers::SelectGroupMembers(const core::String& parameters) :
	IFilter("SelectGroupMembers", parameters) {
	if (_parameters.empty()) {
		_groupId = -1;
	} else {
		_groupId = core::string::toInt(_parameters);
	}
}

void SelectGroupMembers::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	auto func = [&] (const AIPtr& ai) {
		entities.push_back(ai->getId());
		return true;
	};
	Zone* zone = entity->getZone();
	GroupMgr& groupMgr = zone->getGroupMgr();
	groupMgr.visit(_groupId, func);
}

}
