/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "cooldown/CooldownType.h"

namespace backend {

/**
 * @ingroup AI
 */
class TriggerCooldown: public ITask {
private:
	cooldown::Type _cooldownId;
public:
	TriggerCooldown(const core::String& name, const core::String& parameters, const ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldown)

	ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;
};

}

