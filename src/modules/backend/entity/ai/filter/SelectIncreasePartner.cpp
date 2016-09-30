/**
 * @file
 */

#include "SelectIncreasePartner.h"
#include "backend/entity/Npc.h"
#include "core/String.h"
#include "core/Common.h"

namespace backend {

SelectIncreasePartner::SelectIncreasePartner(const std::string& parameters) :
		IFilter("SelectIncreasePartner", parameters) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

void SelectIncreasePartner::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	entities.clear();
	backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (chr.entityType() != e->entityType()) {
			return;
		}
		if (e->cooldownMgr().isCooldown(_cooldownId)) {
			return;
		}
		entities.push_back(e->id());
	});
}

}
