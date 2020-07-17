/**
 * @file
 */

#pragma once

#include "ICondition.h"
#include "cooldown/CooldownType.h"

namespace backend {

/**
 * @ingroup AI
 */
class IsOnCooldown: public ICondition {
private:
	cooldown::Type _cooldownId;
public:
	IsOnCooldown(const core::String& parameters);
	CONDITION_FACTORY(IsOnCooldown)

	bool evaluate(const AIPtr& entity) override;
};

}
