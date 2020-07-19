/**
 * @file
 */

#include "SelectHighestAggro.h"
#include "backend/entity/ai/aggro/AggroMgr.h"

namespace backend {

void SelectHighestAggro::filter (const AIPtr& entity) {
	const EntryPtr entry = entity->getAggroMgr().getHighestEntry();
	if (!entry)
		return;

	const ai::CharacterId id = entry->getCharacterId();
	getFilteredEntities(entity).push_back(id);
}

}
