/**
 * @file
 */

#include "SetPointOfInterest.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK_IMPL(SetPointOfInterest) {
	Npc& npc = getNpc(entity);
	// TODO: poi type
	const poi::Type type = poi::Type::GENERIC;
	npc.setPointOfInterest(type);
	return ai::TreeNodeStatus::FINISHED;
}

}

