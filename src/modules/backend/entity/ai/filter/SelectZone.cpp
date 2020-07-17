/**
 * @file
 */
 
#include "SelectZone.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

SelectZone::SelectZone(const core::String& parameters) :
	IFilter("SelectZone", parameters) {
}

void SelectZone::filter (const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	auto func = [&] (const AIPtr& ai) {
		entities.push_back(ai->getId());
		return true;
	};
	entity->getZone()->execute(func);
}

}
