/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(Die) {
	Npc& npc = chr.getNpc();
	if (npc.die()) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

