/**
 * @file
 */

#include "attrib/ShadowAttributes.h"
#if 0
#include "voxelutil/AStarPathfinder.h"
#endif
#include "Npc.h"
#include "ai/AICharacter.h"
#include "ai/AI.h"
#include "ai/zone/Zone.h"
#include "ai/common/Random.h"
#include "backend/entity/ai/tree/TreeNode.h"
#include "backend/world/Map.h"
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

namespace backend {

std::atomic<EntityId> Npc::_nextNpcId(0);

Npc::Npc(network::EntityType type, const TreeNodePtr& behaviour,
		const MapPtr& map, const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider) :
		Super(_nextNpcId++, map, messageSender, timeProvider, containerProvider),
		_cooldowns(timeProvider, cooldownProvider) {
	_entityType = type;
	_random.setSeed((unsigned int)_entityId);
	_ai = std::make_shared<AI>(behaviour);
	_aiChr = core::make_shared<AICharacter>(_entityId, *this);
	_ai->setCharacter(_aiChr);
	_aiChr->setOrientation(randomf(glm::two_pi<float>()));
	_aiChr->setMetaAttribute(ai::attributes::NAME, this->type());
	_aiChr->setMetaAttribute(ai::attributes::ID, core::string::format(PRIEntId, _entityId));
}

Npc::~Npc() {
}

void Npc::shutdown() {
	Zone* zone = _ai->getZone();
	if (zone != nullptr) {
		zone->destroyAI(id());
	}
	_ai->setZone(nullptr);
	_ai->setCharacter(AICharacterPtr());
	Super::shutdown();
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
	const glm::vec3 spawnPos(randomPos.x, randomPos.y, randomPos.z);
	setHomePosition(spawnPos);
	setTargetPosition(spawnPos);
	_aiChr->setPosition(spawnPos);
	init();
}

double Npc::applyDamage(Entity* attacker, double damage) {
	double health = current(attrib::Type::HEALTH);
	if (health > 0.0) {
		health = core_max(0.0, health - damage);
		if (attacker != nullptr) {
			_ai->getAggroMgr().addAggro(attacker->id(), (float)damage);
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
	_time += dt;
	if (!Super::update(dt)) {
		return false;
	}

	// TODO: attrib for passive aggro
	if (true) {
		visitVisible([&] (const EntityPtr& e) {
			AggroMgr& aggro = ai()->getAggroMgr();
			aggro.addAggro(e->id(), (float)((double)dt / 1000.0));
		});
	}

	_cooldowns.update();

	updateFromAIState();

	moveToGround();

	updateAIState();
	return !dead();
}

void Npc::updateFromAIState() {
	setOrientation(_aiChr->getOrientation());
	setPos(_aiChr->getPosition());
}

void Npc::updateAIState() {
	_aiChr->setPosition(pos());
	for (int i = 0; i <= (int)attrib::Type::MAX; ++i) {
		const attrib::Type attribType = (attrib::Type)i;
		_aiChr->setCurrent(attribType, _attribs.current(attribType));
		_aiChr->setMax(attribType, _attribs.max(attribType));
	}
}

bool Npc::route(const glm::vec3& target) {
#if 0
	std::list<glm::ivec3> result;
	const glm::vec3& pos = _aiChr->getPosition();
	const glm::ivec3 start(pos);
	const glm::ivec3 end(target.x, target.y, target.z);
	if (!_map->findPath(start, end, result)) {
		return false;
	}
	// TODO: use the route
	setTargetPosition(target);
#endif
	return true;
}

void Npc::moveToGround() {
	glm::vec3 pos = this->pos();
	const voxelutil::FloorTraceResult& trace = _map->findFloor(pos);
	if (!trace.isValid()) {
		Log::error("Could not find a valid floor position for the npc");
		return;
	}
	pos.y = (float)trace.heightLevel;
	setPos(pos);
}

}
