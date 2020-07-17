/**
 * @file
 */

#include "TriggerCooldownOnSelection.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

TriggerCooldownOnSelection::TriggerCooldownOnSelection(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		Task(name, parameters, condition) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

ai::TreeNodeStatus TriggerCooldownOnSelection::doAction(AICharacter& chr, int64_t deltaMillis) {
	const FilteredEntities& selection = chr.getNpc().ai()->getFilteredEntities();
	if (selection.empty()) {
		return ai::TreeNodeStatus::FAILED;
	}
	Zone* zone = chr.getNpc().ai()->getZone();
	if (zone == nullptr) {
		return ai::TreeNodeStatus::FAILED;
	}
	for (ai::CharacterId id : selection) {
		auto func = [=] (const AIPtr& ai) {
			Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
			npc.cooldownMgr().triggerCooldown(_cooldownId);
		};
		zone->executeAsync(id, func);
	}
	return ai::TreeNodeStatus::FINISHED;
}

}
