/**
 * @file
 */

#include "SetPointOfInterest.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK_IMPL(SetPointOfInterest) {
	Npc& npc = getNpc(entity);
	npc.setPointOfInterest();
	return ai::TreeNodeStatus::FINISHED;
}

}

