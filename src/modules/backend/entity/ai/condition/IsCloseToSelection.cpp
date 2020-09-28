/**
 * @file
 */

#include "IsCloseToSelection.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/zone/Zone.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/geometric.hpp>

namespace backend {

IsCloseToSelection::IsCloseToSelection(const core::String& parameters) :
		ICondition("IsCloseToSelection", parameters) {
	if (_parameters.empty()) {
		_distance = 1;
	} else {
		_distance = core::string::toInt(_parameters);
	}
	_distance = glm::pow(_distance, 2.0);
}

bool IsCloseToSelection::evaluate(const AIPtr& entity) {
	Zone* zone = entity->getZone();
	if (zone == nullptr) {
		return state(false);
	}

	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty()) {
		return state(false);
	}

	bool foundValidResult = true;
	const glm::vec3& ownPos = entity->getCharacter()->getPosition();
	for (ai::CharacterId id : selection) {
		const AIPtr& ai = zone->getAI(id);
		if (!ai) {
			foundValidResult = false;
			continue;
		}
		foundValidResult = true;
		const glm::vec3& pos = ai->getCharacter()->getPosition();
		const float distance = glm::distance2(pos, ownPos);
		if (distance > (float)_distance) {
			return state(false);
		}
	}
	return state(foundValidResult);
}

}
