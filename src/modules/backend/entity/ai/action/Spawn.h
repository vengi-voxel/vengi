/**
 * @file
 */

#pragma once

#include "backend/entity/ai/tree/ITask.h"
#include "core/Common.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

/**
 * @ingroup AI
 */
AI_TASK(Spawn) {
	Npc& npc = entity->getCharacterCast<AICharacter>().getNpc();
	const glm::ivec3 pos(npc.pos());
	SpawnMgr& spawnMgr = npc.map()->spawnMgr();
	if (spawnMgr.spawn(npc.entityType(), 1, &pos) == 1) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

