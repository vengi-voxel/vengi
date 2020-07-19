/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(Die) {
	Npc& npc = getNpc(entity);
	if (npc.die()) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

