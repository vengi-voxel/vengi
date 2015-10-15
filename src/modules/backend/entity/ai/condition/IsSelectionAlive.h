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
		bool alive = false;
		auto func = [&] (const AIPtr& ai) {
			AICharacter& chr = ai->getCharacterCast<AICharacter>();
			alive = !chr.getNpc().dead();
		};
		if (!entity->getZone()->execute(selection[0], func))
			return false;
		return alive;
	}
};

CONDITION_FACTORY_IMPL(IsSelectionAlive);

}
