/**
 * @file
 */

#include "Steering.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/common/Math.h"
#include "backend/entity/ai/ICharacter.h"

namespace backend {
namespace movement {

bool SelectionSteering::getSelectionTarget(const AIPtr& entity, size_t index, glm::vec3& position) const {
	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty() || selection.size() <= index) {
		return false;
	}
	const Zone* zone = entity->getZone();
	const ai::CharacterId characterId = selection[index];
	const AIPtr& ai = zone->getAI(characterId);
	if (!ai) {
		return false;
	}
	const ICharacterPtr character = ai->getCharacter();
	position = character->getPosition();
	return true;
}

}
}
