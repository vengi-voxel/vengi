/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "cooldown/CooldownType.h"
#include "core/Common.h"
#include "core/String.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

using namespace ai;

namespace backend {

class IsOnCooldown: public ICondition {
private:
	cooldown::Type _cooldownId;
public:
	IsOnCooldown(const std::string& parameters) :
			ICondition("IsOnCooldown", parameters) {
		_cooldownId = static_cast<cooldown::Type>(core::string::toInt(parameters));
		core_assert(_cooldownId != cooldown::Type::NONE);
	}
	CONDITION_FACTORY(IsOnCooldown)

	bool evaluate(const AIPtr& entity) override {
		const AICharacter& chr = ai::character_cast<AICharacter>(entity->getCharacter());
		return chr.getNpc().cooldownMgr().isCooldown(_cooldownId);
	}

};

}
