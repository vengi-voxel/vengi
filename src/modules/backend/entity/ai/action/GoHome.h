/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

using namespace ai;

namespace backend {

AI_TASK(GoHome) {
	backend::Npc& npc = chr.getNpc();
	if (npc.route(npc.homePosition())) {
		return FINISHED;
	}
	return FAILED;
}

}

