/**
 * @file
 */

#include "AICharacter.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/aggro/AggroMgr.h"
#include "backend/entity/ai/common/Random.h"
#include "core/StringUtil.h"
#include "core/Trace.h"

namespace backend {

AICharacter::AICharacter(ai::CharacterId id, Npc& npc) :
		Super(id), _npc(npc) {
}

AICharacter::~AICharacter() {
}

void AICharacter::setPosition(const glm::vec3& position) {
	Super::setPosition(position);
	_npc.setPos(position);
}

void AICharacter::setOrientation(float orientation) {
	Super::setOrientation(orientation);
	_npc.setOrientation(orientation);
}

}
