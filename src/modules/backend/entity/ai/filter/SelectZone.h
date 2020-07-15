/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

/**
 * @brief This filter will pick the entities from the zone of the given entity
 */
class SelectZone: public IFilter {
public:
	FILTER_FACTORY(SelectZone)

	explicit SelectZone(const core::String& parameters = "") :
		IFilter("SelectZone", parameters) {
	}

	void filter (const AIPtr& entity) override {
		FilteredEntities& entities = getFilteredEntities(entity);
		auto func = [&] (const AIPtr& ai) {
			entities.push_back(ai->getId());
			return true;
		};
		entity->getZone()->execute(func);
	}
};

}
