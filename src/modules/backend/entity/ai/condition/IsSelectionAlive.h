#pragma once

#include "backend/entity/ai/AICommon.h"

using namespace ai;

namespace backend {

class IsSelectionAlive: public ICondition {
protected:
	CONDITION_CLASS(IsSelectionAlive)
public:
	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override {
		const FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty())
			return true;
		const Zone* zone = entity->getZone();
		const AIPtr& ai = zone->getAI(selection[0]);
		AICharacter& chr = ai->getCharacterCast<AICharacter>();
		return !chr.getNpc().dead();
	}
};

CONDITION_FACTORY_IMPL(IsSelectionAlive);

}
