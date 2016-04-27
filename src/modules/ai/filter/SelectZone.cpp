#include "SelectZone.h"
#include "zone/Zone.h"

namespace ai {

void SelectZone::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	auto func = [&] (const AIPtr& ai) {
		entities.push_back(ai->getId());
		return true;
	};
	entity->getZone()->execute(func);
}

FILTER_FACTORY_IMPL(SelectZone)

}
