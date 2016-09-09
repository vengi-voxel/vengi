/**
 * @file
 */

#include "TriggerCooldownOnSelection.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"

namespace backend {

TriggerCooldownOnSelection::TriggerCooldownOnSelection(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
		Task(name, parameters, condition) {
	_cooldownId = static_cast<cooldown::Type>(core::string::toInt(parameters));
	core_assert(_cooldownId != cooldown::Type::NONE);
}

TreeNodeStatus TriggerCooldownOnSelection::doAction(backend::AICharacter& chr, int64_t deltaMillis) {
	const FilteredEntities& selection = chr.getNpc().ai()->getFilteredEntities();
	if (selection.empty())
		return FAILED;
	ai::Zone* zone = chr.getNpc().ai()->getZone();
	if (zone == nullptr)
		return FAILED;
	for (CharacterId id : selection) {
		auto func = [=] (const ai::AIPtr& ai) {
			Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
			npc.cooldownMgr().triggerCooldown(_cooldownId);
		};
		zone->executeAsync(id, func);
	}
	return FINISHED;
}

}
