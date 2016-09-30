/**
 * @file
 */

#pragma once

#include "Task.h"

using namespace ai;

namespace backend {

AI_TASK(Die) {
	backend::Npc& npc = chr.getNpc();
	if (npc.die()) {
		return FINISHED;
	}
	return FAILED;
}

}

