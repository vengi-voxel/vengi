#pragma once

#include <conditions/ICondition.h>
#include <common/String.h>
#include "backend/entity/EntityStorage.h"

using namespace ai;

namespace backend {

class IsCloseToSelection: public ICondition {
protected:
	int _distance;

	IsCloseToSelection(const std::string& parameters) :
			ICondition("IsCloseToSelection", parameters) {
		if (_parameters.empty())
			_distance = 1;
		else
			_distance = std::stoi(_parameters);
	}
public:
	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override {
		ai::Zone* zone = entity->getZone();
		if (zone == nullptr)
			return false;

		const FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty())
			return false;

		auto func = [&] (const ai::AIPtr& ai) -> bool {
			const Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
			const glm::vec3& pos = npc.pos();
			const ai::Vector3f& ownPos = entity->getCharacter()->getPosition();
			const int distance = ownPos.distance(ai::Vector3f(pos.x, pos.y, pos.z));
			return distance <= _distance;
		};

		const AIPtr& selected = zone->getAI(selection[0]);
		auto future = zone->executeAsync(selected, func);
		return future.get();
	}
};

CONDITION_FACTORY_IMPL(IsCloseToSelection)

}
