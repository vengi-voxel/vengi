/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(GoHome) {
	backend::Npc& npc = chr.getNpc();
	if (npc.route(npc.homePosition())) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

