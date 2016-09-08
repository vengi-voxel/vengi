/**
 * @file
 */

#include "SelectIncreasePartner.h"
#include "backend/entity/Npc.h"
#include "core/Log.h"

namespace backend {

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
