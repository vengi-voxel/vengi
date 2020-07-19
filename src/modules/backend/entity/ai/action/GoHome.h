/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(GoHome) {
	Npc& npc = getNpc(entity);
	if (npc.route(npc.homePosition())) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

