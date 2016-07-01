/**
 * @file
 */

#pragma once

#include "Task.h"
#include "cooldown/CooldownType.h"

using namespace ai;

namespace backend {

class TriggerCooldown: public Task {
private:
	cooldown::CooldownType _cooldownId;
public:
	TriggerCooldown(const std::string& name, const std::string& parameters, const ConditionPtr& condition);
	NODE_FACTORY(TriggerCooldown)

	TreeNodeStatus doAction(backend::AICharacter& chr, long deltaMillis) override;
};

}

