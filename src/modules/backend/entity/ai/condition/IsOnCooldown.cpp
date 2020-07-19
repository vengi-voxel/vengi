/**
 * @file
 */

#include "IsOnCooldown.h"
#include "core/Log.h"
#include "core/Common.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/Npc.h"

namespace backend {

IsOnCooldown::IsOnCooldown(const core::String& parameters) :
		ICondition("IsOnCooldown", parameters) {
	_cooldownId = cooldown::getType(parameters);
	core_assert_always(_cooldownId != cooldown::Type::NONE);
}

bool IsOnCooldown::evaluate(const AIPtr& entity) {
	Npc& npc = getNpc(entity);
	return npc.cooldownMgr().isCooldown(_cooldownId);
}

}
