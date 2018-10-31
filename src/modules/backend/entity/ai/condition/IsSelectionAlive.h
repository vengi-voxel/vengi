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
class IsSelectionAlive: public ai::ICondition {
protected:
public:
	CONDITION_CLASS(IsSelectionAlive)
	CONDITION_FACTORY(IsSelectionAlive)

	bool evaluate(const ai::AIPtr& entity) override {
		const ai::Zone* zone = entity->getZone();
		if (zone == nullptr) {
			return false;
		}
		const ai::FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty()) {
			return false;
		}
		for (ai::CharacterId id : selection) {
			const ai::AIPtr& ai = zone->getAI(id);
			const AICharacter& chr = ai->getCharacterCast<AICharacter>();
			if (chr.getNpc().dead()) {
				return false;
			}
		}
		return true;
	}
};

}
