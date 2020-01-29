/**
 * @file
 */

#include "SelectIncreasePartner.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/Assert.h"

namespace backend {

SelectIncreasePartner::SelectIncreasePartner(const std::string& parameters) :
		ai::IFilter("SelectIncreasePartner", parameters) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

void SelectIncreasePartner::filter(const ai::AIPtr& entity) {
	ai::FilteredEntities& entities = getFilteredEntities(entity);
	entities.clear();
	backend::Npc& chr = getNpc(entity);
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (chr.entityType() != e->entityType()) {
			return;
		}
		const NpcPtr& npc = std::static_pointer_cast<Npc>(e);
		if (npc->cooldownMgr().isCooldown(_cooldownId)) {
			return;
		}
		entities.push_back(e->id());
	});
}

}
