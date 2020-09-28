/**
 * @file
 */

#include "TriggerCooldown.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

TriggerCooldown::TriggerCooldown(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		ITask(name, parameters, condition) {
	_type = "TriggerCooldown";
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

ai::TreeNodeStatus TriggerCooldown::doAction(const AIPtr& entity, int64_t deltaMillis) {
	Npc& npc = getNpc(entity);
	const cooldown::CooldownTriggerState s = npc.cooldownMgr().triggerCooldown(_cooldownId);
	if (s == cooldown::CooldownTriggerState::SUCCESS) {
		return ai::TreeNodeStatus::FINISHED;
	} else if (s == cooldown::CooldownTriggerState::FAILED) {
		return ai::TreeNodeStatus::EXCEPTION;
	}
	return ai::TreeNodeStatus::FAILED;
}

}
