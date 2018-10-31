/**
 * @file
 */

#include "SelectVisible.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

void SelectVisible::filter(const ai::AIPtr& entity) {
	ai::FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = getNpc(entity);
	chr.visitVisible([&] (const EntityPtr& e) {
		entities.push_back(e->id());
	});
}

}
