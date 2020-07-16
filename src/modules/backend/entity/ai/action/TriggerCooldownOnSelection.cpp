/**
 * @file
 */

#include "TriggerCooldownOnSelection.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

TriggerCooldownOnSelection::TriggerCooldownOnSelection(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		ITask(name, parameters, condition) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

ai::TreeNodeStatus TriggerCooldownOnSelection::doAction(const AIPtr& entity, int64_t deltaMillis) {
	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty()) {
		return ai::TreeNodeStatus::FAILED;
	}
	Zone* zone = entity->getZone();
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
