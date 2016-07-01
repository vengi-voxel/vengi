/**
 * @file
 */

#include "SelectVisible.h"
#include "backend/entity/Npc.h"

namespace backend {

void SelectVisible::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		entities.push_back(e->id());
	});
}

}
