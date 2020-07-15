/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter will pick the entity with the highest aggro value
 */
class SelectHighestAggro: public IFilter {
public:
	FILTER_CLASS_SINGLETON(SelectHighestAggro)

	void filter (const AIPtr& entity) override;
};

inline void SelectHighestAggro::filter (const AIPtr& entity) {
	const EntryPtr entry = entity->getAggroMgr().getHighestEntry();
	if (!entry)
		return;

	const ai::CharacterId id = entry->getCharacterId();
	getFilteredEntities(entity).push_back(id);
}

}
