/**
 * @file
 */

#include "AttackMgr.h"
#include "backend/world/Map.h"
#include "backend/entity/Npc.h"

namespace backend {

AttackMgr::AttackMgr(Map* map) :
		_map(map) {
}

bool AttackMgr::stopAttack(EntityId attackerId, EntityId victimId) {
	return true;
}

// TODO: users as victims and attackers...
bool AttackMgr::startAttack(EntityId attackerId, EntityId victimId) {
	const NpcPtr& attacker = _map->npc(attackerId);
	if (!attacker) {
		return false;
	}
	const double strength = attacker->current(attrib::Type::STRENGTH);
	if (strength <= 0.0) {
		return false;
	}
	const NpcPtr& npc = _map->npc(victimId);
	if (!npc) {
		return false;
	}
	return npc->applyDamage(attacker.get(), strength) > 0.0;
}

bool AttackMgr::init() {
	return true;
}

void AttackMgr::shutdown() {
}

// TODO: this must get ticket more often than the 'normal' map tick.
void AttackMgr::update(long dt) {
}

}
