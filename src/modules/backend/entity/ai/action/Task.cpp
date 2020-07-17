/**
 * @file
 */

#include "Task.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AI.h"

namespace backend {

ai::TreeNodeStatus Task::doAction(const AIPtr& entity, int64_t deltaMillis) {
	return doAction(character_cast<AICharacter>(entity->getCharacter()), deltaMillis);
}

}
