/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

/**
 * @ingroup AI
 */
class IsSelectionAlive: public ICondition {
public:
	CONDITION_CLASS(IsSelectionAlive)
	CONDITION_FACTORY(IsSelectionAlive)

	bool evaluate(const AIPtr& entity) override {
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
};

}
