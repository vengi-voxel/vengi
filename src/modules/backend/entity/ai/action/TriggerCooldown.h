/**
 * @file
 */

#pragma once

#include "Task.h"
#include "cooldown/CooldownType.h"

namespace backend {

/**
 * @ingroup AI
 */
class TriggerCooldown: public Task {
private:
	cooldown::Type _cooldownId;
public:
	TriggerCooldown(const core::String& name, const core::String& parameters, const ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldown)

	ai::TreeNodeStatus doAction(AICharacter& chr, int64_t deltaMillis) override;
};

}

