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
	for (ai::CharacterId id : selection) {
		const AIPtr& ai = zone->getAI(id);
		const AICharacter& chr = ai->getCharacterCast<AICharacter>();
		if (chr.getNpc().dead()) {
			return false;
		}
	}
	return true;
}

}
