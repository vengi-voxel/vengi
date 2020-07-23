/**
 * @file
 */

#include "Spawn.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

/**
 * @ingroup AI
 */
AI_TASK_IMPL(Spawn) {
	Npc& npc = getNpc(entity);
	const glm::ivec3 pos = entity->getCharacter()->getPosition();
	SpawnMgr& spawnMgr = npc.map()->spawnMgr();
	// TODO: amount, type and radius
	if (spawnMgr.spawn(npc.entityType(), 1, &pos) == 1) {
		return ai::TreeNodeStatus::FINISHED;
	}
	return ai::TreeNodeStatus::FAILED;
}

}

