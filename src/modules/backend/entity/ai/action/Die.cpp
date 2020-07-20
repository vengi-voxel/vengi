/**
 * @file
 */

#include "Die.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK_IMPL(Die) {
	Npc& npc = getNpc(entity);
	if (npc.die()) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

