#include "SelectGroupMembers.h"
#include "zone/Zone.h"

namespace ai {

void SelectGroupMembers::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	auto func = [&] (const AIPtr& ai) {
		entities.push_back(ai->getId());
		return true;
	};
	entity->getZone()->getGroupMgr().visit(_groupId, func);
}

FILTER_FACTORY_IMPL(SelectGroupMembers)

}
