/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"
#include "zone/Zone.h"

namespace ai {

/**
 * @brief This filter will pick the group leader of the specified group
 */
class SelectGroupLeader: public IFilter {
protected:
	GroupId _groupId;
public:
	FILTER_FACTORY(SelectGroupLeader)

	explicit SelectGroupLeader(const core::String& parameters = "") :
		IFilter("SelectGroupLeader", parameters) {
		if (_parameters.empty()) {
			_groupId = -1;
		} else {
			_groupId = std::stoi(_parameters);
		}
	}

	void filter (const AIPtr& entity) override {
		FilteredEntities& entities = getFilteredEntities(entity);
		const Zone* zone = entity->getZone();
		const GroupMgr& groupMgr = zone->getGroupMgr();
		const AIPtr& groupLeader = groupMgr.getLeader(_groupId);
		if (groupLeader) {
			entities.push_back(groupLeader->getId());
		}
	}
};

}
