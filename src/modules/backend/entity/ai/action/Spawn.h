/**
 * @file
 */

#pragma once

#include "Task.h"
#include "core/Common.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"

namespace backend {

/**
 * @ingroup AI
 */
AI_TASK(Spawn) {
	backend::Npc& npc = chr.getNpc();
	const glm::ivec3 pos(npc.pos());
	SpawnMgr& spawnMgr = npc.map()->spawnMgr();
	if (spawnMgr.spawn(npc.entityType(), 1, &pos) == 1) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

