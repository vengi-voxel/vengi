/**
 * @file
 */

#pragma once

#include "Task.h"
#include "cooldown/CooldownType.h"
#include "backend/entity/EntityStorage.h"

namespace backend {

/**
 * @ingroup AI
 */
class TriggerCooldownOnSelection: public Task {
private:
	cooldown::Type _cooldownId;
public:
	TriggerCooldownOnSelection(const std::string& name, const std::string& parameters, const ai::ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldownOnSelection)

	ai::TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) override;
};

}

