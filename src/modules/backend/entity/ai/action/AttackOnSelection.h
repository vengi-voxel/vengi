/**
 * @file
 */

#pragma once

#include "Task.h"

using namespace ai;

namespace backend {

AI_TASK(AttackOnSelection) {
	backend::Npc& npc = chr.getNpc();
	const FilteredEntities& selection = npc.ai()->getFilteredEntities();
	if (selection.empty()) {
		return FAILED;
	}
	if (npc.attack(selection[0])) {
		return FINISHED;
	}
	return FAILED;
}

}

