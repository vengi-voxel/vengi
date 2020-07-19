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

glm::vec3 SelectionSteering::getSelectionTarget(const AIPtr& entity, size_t index) const {
	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty() || selection.size() <= index) {
		return VEC3_INFINITE;
	}
	const Zone* zone = entity->getZone();
	const ai::CharacterId characterId = selection[index];
	const AIPtr& ai = zone->getAI(characterId);
	if (!ai) {
		return VEC3_INFINITE;
	}
	const ICharacterPtr character = ai->getCharacter();
	return character->getPosition();
}

}
}
