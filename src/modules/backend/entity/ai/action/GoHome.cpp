/**
 * @file
 */

#include "GoHome.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK_IMPL(GoHome) {
	Npc& npc = getNpc(entity);
	if (npc.route(npc.homePosition())) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

