/**
 * @file
 */

#include "TriggerCooldown.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"

namespace backend {

TriggerCooldown::TriggerCooldown(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
		Task(name, parameters, condition) {
	_cooldownId = static_cast<cooldown::CooldownType>(core::string::toInt(parameters));
	core_assert(_cooldownId > cooldown::NONE);
	core_assert(_cooldownId < cooldown::MAX);
}

TreeNodeStatus TriggerCooldown::doAction(backend::AICharacter& chr, long deltaMillis) {
	backend::Npc& npc = chr.getNpc();
	if (npc.cooldownMgr().triggerCooldown(_cooldownId) == cooldown::CooldownTriggerState::SUCCESS) {
		return FINISHED;
	}
	return FAILED;
}

NODE_FACTORY_IMPL(TriggerCooldown)

}
