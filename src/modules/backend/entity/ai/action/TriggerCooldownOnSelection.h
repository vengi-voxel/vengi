/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "cooldown/CooldownType.h"
#include "backend/entity/EntityStorage.h"

namespace backend {

/**
 * @ingroup AI
 */
class TriggerCooldownOnSelection: public ITask {
private:
	cooldown::Type _cooldownId;
public:
	TriggerCooldownOnSelection(const core::String& name, const core::String& parameters, const ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldownOnSelection)

	ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;
};

}

