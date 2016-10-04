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
		const AIPtr& ai = zone->getAI(selection[0]);
		AICharacter& chr = ai->getCharacterCast<AICharacter>();
		return !chr.getNpc().dead();
	}
};

}
