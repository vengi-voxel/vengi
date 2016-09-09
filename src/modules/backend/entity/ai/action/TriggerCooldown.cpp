/**
 * @file
 */

#include "TriggerCooldown.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"

namespace backend {

TriggerCooldown::TriggerCooldown(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
		Task(name, parameters, condition) {
	_cooldownId = static_cast<cooldown::Type>(core::string::toInt(parameters));
	core_assert(_cooldownId != cooldown::Type::NONE);
}

TreeNodeStatus TriggerCooldown::doAction(backend::AICharacter& chr, int64_t deltaMillis) {
	backend::Npc& npc = chr.getNpc();
	const cooldown::CooldownTriggerState s = npc.cooldownMgr().triggerCooldown(_cooldownId);
	if (s == cooldown::CooldownTriggerState::SUCCESS) {
		return FINISHED;
	}
	return FAILED;
}

}
