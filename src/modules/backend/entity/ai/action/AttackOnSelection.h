/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(AttackOnSelection) {
	const FilteredEntities& selection = entity->getFilteredEntities();
	if (selection.empty()) {
		return ai::TreeNodeStatus::FAILED;
	}
	bool attacked = false;
	Npc& npc = getNpc(entity);
	for (ai::CharacterId id : selection) {
		attacked |= npc.attack(id);
	}
	return attacked ? ai::TreeNodeStatus::FINISHED : ai::TreeNodeStatus::FAILED;
}

}

