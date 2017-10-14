/**
 * @file
 */

#include "SelectVisible.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

void SelectVisible::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
	chr.visitVisible([&] (const EntityPtr& e) {
		entities.push_back(e->id());
	});
}

}
