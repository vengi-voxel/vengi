/**
 * @file
 */

#include "Steering.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/common/Math.h"
#include "backend/entity/ai/ICharacter.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

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

MoveVector ISteering::seek(const glm::vec3& pos, const glm::vec3& target, float speed) const {
	const glm::vec3& dist = target - pos;
	const float dot = glm::length2(dist);
	if (dot <= glm::epsilon<float>()) {
		return MoveVector::Invalid;
	}
	const glm::vec3& v = dist * glm::inversesqrt(dot);
	const float orientation = angle(v);
	return {v * speed, orientation};
}

}
}
