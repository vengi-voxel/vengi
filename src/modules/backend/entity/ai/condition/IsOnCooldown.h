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
	cooldown::CooldownType _cooldownId;
	IsOnCooldown(const std::string& parameters) :
			ICondition("IsOnCooldown", parameters) {
		_cooldownId = static_cast<cooldown::CooldownType>(core::string::toInt(parameters));
		core_assert(_cooldownId > cooldown::NONE);
		core_assert(_cooldownId < cooldown::MAX);
	}
public:
	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override {
		const AICharacter& chr = ai::character_cast<AICharacter>(entity->getCharacter());
		return chr.getNpc().cooldownMgr().isCooldown(_cooldownId);
	}

};

CONDITION_FACTORY_IMPL(IsOnCooldown)

}
