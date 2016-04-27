#include "SelectGroupLeader.h"
#include "zone/Zone.h"

namespace ai {

void SelectGroupLeader::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	const AIPtr& groupLeader = entity->getZone()->getGroupMgr().getLeader(_groupId);
	if (groupLeader) {
		entities.push_back(groupLeader->getId());
	}
}

FILTER_FACTORY_IMPL(SelectGroupLeader)

}
