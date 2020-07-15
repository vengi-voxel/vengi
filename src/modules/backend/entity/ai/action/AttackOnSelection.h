/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

namespace backend {

AI_TASK(AttackOnSelection) {
	Npc& npc = chr.getNpc();
	const FilteredEntities& selection = npc.ai()->getFilteredEntities();
	if (selection.empty()) {
		return ai::TreeNodeStatus::FAILED;
	}
	bool attacked = false;
	for (ai::CharacterId id : selection) {
		attacked |= npc.attack(id);
	}
	return attacked ? ai::TreeNodeStatus::FINISHED : ai::TreeNodeStatus::FAILED;
}

}

