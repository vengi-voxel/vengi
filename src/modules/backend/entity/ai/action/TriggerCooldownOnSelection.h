/**
 * @file
 */

#pragma once

#include "Task.h"
#include "cooldown/CooldownType.h"
#include "backend/entity/EntityStorage.h"

using namespace ai;

namespace backend {

class TriggerCooldownOnSelection: public Task {
private:
	cooldown::Type _cooldownId;
public:
	TriggerCooldownOnSelection(const std::string& name, const std::string& parameters, const ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldownOnSelection)

	TreeNodeStatus doAction(backend::AICharacter& chr, long deltaMillis) override;
};

}

