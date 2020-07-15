/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "cooldown/CooldownType.h"
#include "core/Common.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

/**
 * @ingroup AI
 */
class IsOnCooldown: public ICondition {
private:
	cooldown::Type _cooldownId;
public:
	IsOnCooldown(const core::String& parameters) :
			ICondition("IsOnCooldown", parameters) {
		_cooldownId = cooldown::getType(parameters);
		core_assert_always(_cooldownId != cooldown::Type::NONE);
	}
	CONDITION_FACTORY(IsOnCooldown)

	bool evaluate(const AIPtr& entity) override {
		const AICharacter& chr = character_cast<AICharacter>(entity->getCharacter());
		return chr.getNpc().cooldownMgr().isCooldown(_cooldownId);
	}
};

}
