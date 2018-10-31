/**
 * @file
 */

#include "TriggerCooldownOnSelection.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"

namespace backend {

TriggerCooldownOnSelection::TriggerCooldownOnSelection(const std::string& name, const std::string& parameters, const ai::ConditionPtr& condition) :
		Task(name, parameters, condition) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

ai::TreeNodeStatus TriggerCooldownOnSelection::doAction(backend::AICharacter& chr, int64_t deltaMillis) {
	const ai::FilteredEntities& selection = chr.getNpc().ai()->getFilteredEntities();
	if (selection.empty()) {
		return ai::TreeNodeStatus::FAILED;
	}
	ai::Zone* zone = chr.getNpc().ai()->getZone();
	if (zone == nullptr) {
		return ai::TreeNodeStatus::FAILED;
	}
	for (ai::CharacterId id : selection) {
		auto func = [=] (const ai::AIPtr& ai) {
			Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
			npc.cooldownMgr().triggerCooldown(_cooldownId);
		};
		zone->executeAsync(id, func);
	}
	return ai::TreeNodeStatus::FINISHED;
}

}
