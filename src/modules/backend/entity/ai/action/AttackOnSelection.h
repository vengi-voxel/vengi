/**
 * @file
 */

#pragma once

#include "Task.h"
#include "backend/entity/Npc.h"

using namespace ai;

namespace backend {

AI_TASK(AttackOnSelection) {
	backend::Npc& npc = chr.getNpc();
	const FilteredEntities& selection = npc.ai()->getFilteredEntities();
	if (selection.empty()) {
		return FAILED;
	}
	bool attacked = false;
	for (CharacterId id : selection) {
		attacked |= npc.attack(id);
	}
	return attacked ? FINISHED : FAILED;
}

}

