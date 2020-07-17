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

namespace backend {

IsCloseToSelection::IsCloseToSelection(const core::String& parameters) :
		ICondition("IsCloseToSelection", parameters) {
	if (_parameters.empty()) {
		_distance = 1;
	} else {
		_distance = core::string::toInt(_parameters);
	}
}

bool IsCloseToSelection::evaluate(const AIPtr& entity) {
	Zone* zone = entity->getZone();
	if (zone == nullptr) {
		return false;
	}

	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty()) {
		return false;
	}

	for (ai::CharacterId id : selection) {
		const AIPtr& ai = zone->getAI(id);
		const Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
		const glm::vec3& pos = npc.pos();
		const glm::vec3& ownPos = entity->getCharacter()->getPosition();
		const float distance = glm::distance(pos, ownPos);
		if (distance > _distance) {
			return false;
		}
	}
	return true;
}

}
