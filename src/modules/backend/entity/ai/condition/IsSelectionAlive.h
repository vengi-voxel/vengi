/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class IsSelectionAlive: public ICondition {
protected:
public:
	CONDITION_CLASS(IsSelectionAlive)
	CONDITION_FACTORY(IsSelectionAlive)

	bool evaluate(const AIPtr& entity) override {
		const FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty()) {
			return false;
		}
		const Zone* zone = entity->getZone();
		for (CharacterId id : selection) {
			const AIPtr& ai = zone->getAI(id);
			AICharacter& chr = ai->getCharacterCast<AICharacter>();
			if (chr.getNpc().dead()) {
				return false;
			}
		}
		return true;
	}
};

}
