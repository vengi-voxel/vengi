/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(SetPointOfInterest) {
	Npc& npc = getNpc(entity);
	npc.setPointOfInterest();
	return ai::TreeNodeStatus::FINISHED;
}

}

