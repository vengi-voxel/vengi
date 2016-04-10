#include "Npc.h"
#include "EntityStorage.h"
#include "backend/poi/PoiProvider.h"
#include "voxel/World.h"
#include "cubiquity/PolyVox/AStarPathfinder.h"

namespace backend {

std::atomic<EntityId> Npc::_nextNpcId(5000000);

Npc::Npc(network::messages::NpcType type, const EntityStoragePtr& entityStorage, const ai::TreeNodePtr& behaviour, const voxel::WorldPtr& world, const network::MessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider, const PoiProviderPtr& poiProvider) :
		Entity(_nextNpcId++, messageSender, timeProvider, containerProvider), _type(type), _humanControlled(false), _world(world), _poiProvider(poiProvider) {
	_ai = ai::AIPtr(new ai::AI(behaviour));
	_ai->setCharacter(ai::ICharacterPtr(new AICharacter(_entityId, *this)));
}

Npc::~Npc() {
	ai::Zone* zone = _ai->getZone();
	if (zone == nullptr)
		return;
	zone->destroyAI(id());
	_ai->setZone(nullptr);
}

void Npc::init(const glm::ivec3* pos) {
	const glm::ivec3& randomPos = pos ? *pos : _world->randomPos();
	const int material = _world->getMaterial(randomPos.x, randomPos.y, randomPos.z);
	Log::info("spawn character %i with behaviour tree %s at position %i:%i:%i (material: %i)",
			ai()->getId(), ai()->getBehaviour()->getName().c_str(), randomPos.x, randomPos.y, randomPos.z, material);
	setHomePosition(randomPos);
	_ai->getCharacter()->setPosition(ai::Vector3f(randomPos.x, randomPos.y, randomPos.z));
	const char *typeName = network::messages::EnumNameNpcType(_type);
	addContainer(typeName);
	initAttribs();
}

std::string Npc::name() const {
	return network::messages::EnumNameNpcType(_type);
}

void Npc::setPointOfInterest() {
	_poiProvider->addPointOfInterest(pos());
}

float Npc::orientation() const {
	return _ai->getCharacter()->getOrientation();
}

double Npc::applyDamage(Npc* attacker, double damage) {
	double health = _attribs.getCurrent(attrib::Types::HEALTH);
	if (health > 0.0) {
		health = std::max(0.0, health - damage);
		if (attacker != nullptr)
			_ai->getAggroMgr().addAggro(attacker->ai(), damage);
		_attribs.setCurrent(attrib::Types::HEALTH, health);
		return damage;
	}
	return 0.0;
}

bool Npc::die() {
	return applyDamage(nullptr, _attribs.getCurrent(attrib::Types::HEALTH)) > 0.0;
}

bool Npc::attack(ai::CharacterId id) {
	const double strength = _attribs.getCurrent(attrib::Types::STRENGTH);
	if (strength <= 0.0)
		return false;
	return _ai->getZone()->executeAsync(id, [=] (const ai::AIPtr & targetAi) {
		AICharacter& targetChr = ai::character_cast<AICharacter>(targetAi->getCharacter());
		targetChr.getNpc().applyDamage(this, strength);
	});
}

glm::vec3 Npc::pos() const {
	const ai::Vector3f pos = _ai->getCharacter()->getPosition();
	return glm::vec3(pos.x, pos.y, pos.z);
}

bool Npc::update(long dt) {
	if (!Entity::update(dt))
		return false;
	_ai->getCharacter()->setSpeed(_attribs.getCurrent(attrib::Types::SPEED));
	return !dead();
}

bool Npc::route(const glm::ivec3& target) {
	std::list<PolyVox::Vector3DInt32> result;
	const ai::Vector3f pos = _ai->getCharacter()->getPosition();
	const PolyVox::Vector3DInt32 start(pos.x, pos.y, pos.z);
	const PolyVox::Vector3DInt32 end(target.x, target.y, target.z);
	return _world->findPath(start, end, result);
}

void Npc::moveToGround() {
	const ai::Vector3f pos = _ai->getCharacter()->getPosition();
	_ai->getCharacter()->setPosition(ai::Vector3f(pos.x, _world->findFloor(pos.x, pos.z), pos.z));
}

}
