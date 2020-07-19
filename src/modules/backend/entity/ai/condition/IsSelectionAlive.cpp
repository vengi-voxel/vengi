/**
 * @file
 */

#include "IsSelectionAlive.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/Npc.h"

namespace backend {

bool IsSelectionAlive::evaluate(const AIPtr& entity) {
	const Zone* zone = entity->getZone();
	if (zone == nullptr) {
		return false;
	}
	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty()) {
		return false;
	}
	bool foundValidResult = true;
	for (ai::CharacterId id : selection) {
		const AIPtr& ai = zone->getAI(id);
		if (!ai) {
			foundValidResult = false;
			continue;
		}
		foundValidResult = true;
		Npc& npc = getNpc(ai);
		if (npc.dead()) {
			return false;
		}
	}
	return foundValidResult;
}

}
