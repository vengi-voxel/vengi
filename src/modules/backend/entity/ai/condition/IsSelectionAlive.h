#pragma once

#include <conditions/ICondition.h>
#include <common/String.h>

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
		static const auto func = [] (const AIPtr& ai) -> bool {
			AICharacter& chr = ai->getCharacterCast<AICharacter>();
			return !chr.getNpc().dead();
		};
		const Zone* zone = entity->getZone();
		const AIPtr& selected = zone->getAI(selection[0]);
		auto future = zone->executeAsync(selected, func);
		return future.get();
	}
};

CONDITION_FACTORY_IMPL(IsSelectionAlive);

}
