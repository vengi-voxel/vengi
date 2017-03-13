/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

using namespace ai;

namespace backend {

AI_TASK(SetPointOfInterest) {
	backend::Npc& npc = chr.getNpc();
	npc.setPointOfInterest();
	return FINISHED;
}

}

