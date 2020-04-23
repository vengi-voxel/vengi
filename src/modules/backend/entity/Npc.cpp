/**
 * @file
 */

#if 0
#include "voxelutil/AStarPathfinder.h"
#endif
#include "Npc.h"
#include "ai/AICharacter.h"
#include "ai/AI.h"
#include "ai/zone/Zone.h"
#include "backend/world/Map.h"

namespace backend {

std::atomic<EntityId> Npc::_nextNpcId(0);

Npc::Npc(network::EntityType type, const ai::TreeNodePtr& behaviour,
		const MapPtr& map, const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider) :
		Super(_nextNpcId++, map, messageSender, timeProvider, containerProvider),
		_cooldowns(timeProvider, cooldownProvider) {
	_entityType = type;
	_ai = std::make_shared<ai::AI>(behaviour);
	_aiChr = std::make_shared<AICharacter>(_entityId, *this);
	_ai->setCharacter(_aiChr);
}

Npc::~Npc() {
}

void Npc::shutdown() {
	ai::Zone* zone = _ai->getZone();
	if (zone == nullptr) {
		return;
	}
	zone->destroyAI(id());
	_ai->setZone(nullptr);
}

void Npc::init() {
	Super::init();
	_ai->getAggroMgr().setReduceByValue(0.1f);
}

void Npc::init(const glm::ivec3* pos) {
	const glm::ivec3& randomPos = pos ? *pos : _map->randomPos();
	Log::info("spawn character %i with behaviour tree %s at position %i:%i:%i",
			ai()->getId(), ai()->getBehaviour()->getName().c_str(),
			randomPos.x, randomPos.y, randomPos.z);
	setHomePosition(randomPos);
	_aiChr->setPosition(glm::vec3(randomPos.x, randomPos.y, randomPos.z));
	init();
}

double Npc::applyDamage(Entity* attacker, double damage) {
	double health = current(attrib::Type::HEALTH);
	if (health > 0.0) {
		health = core_max(0.0, health - damage);
		if (attacker != nullptr) {
			_ai->getAggroMgr().addAggro(attacker->id(), damage);
		}
		setCurrent(attrib::Type::HEALTH, health);
		return damage;
	}
	return 0.0;
}

bool Npc::die() {
	return applyDamage(nullptr, current(attrib::Type::HEALTH)) > 0.0;
}

bool Npc::update(long dt) {
	core_trace_scoped(NpcUpdate);
	if (!Super::update(dt)) {
		return false;
	}
	_cooldowns.update();
	const ai::ICharacterPtr& character = _ai->getCharacter();
	character->setSpeed(current(attrib::Type::SPEED));
	character->setOrientation(orientation());
	return !dead();
}

bool Npc::route(const glm::ivec3& target) {
#if 0
	std::list<glm::ivec3> result;
	const glm::vec3& pos = _aiChr->getPosition();
	const glm::ivec3 start(pos);
	const glm::ivec3 end(target.x, target.y, target.z);
	if (!_map->findPath(start, end, result)) {
		return false;
	}
	// TODO: use the route
#endif
	return true;
}

void Npc::moveToGround() {
	glm::vec3 pos = _aiChr->getPosition();
	pos.y = _map->findFloor(pos);
	_aiChr->setPosition(pos);
}

}
