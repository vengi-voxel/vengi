/**
 * @file
 */

#include "User.h"
#include "core/Var.h"
#include "voxel/World.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id, const std::string& name, const network::MessageSenderPtr& messageSender,
		const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider, const poi::PoiProviderPtr& poiProvider) :
		Entity(id, messageSender, timeProvider, containerProvider, cooldownProvider),
		_name(name), _world(world), _poiProvider(poiProvider) {
	setPeer(peer);
	const glm::vec3& poi = _poiProvider->getPointOfInterest();
	_pos = poi;
	_entityType = network::EntityType::PLAYER;
	_userTimeout = core::Var::getSafe(cfg::ServerUserTimeout);
}

void User::visibleAdd(const EntitySet& entities) {
	Entity::visibleAdd(entities);
	for (const EntityPtr& e : entities) {
		sendEntitySpawn(e);
	}
}

void User::visibleRemove(const EntitySet& entities) {
	Entity::visibleRemove(entities);
	for (const EntityPtr& e : entities) {
		sendEntityRemove(e);
	}
}

ENetPeer* User::setPeer(ENetPeer* peer) {
	ENetPeer* old = _peer;
	_peer = peer;
	if (_peer) {
		_host = _peer->address.host;
		_peer->data = this;
	} else {
		_host = 0u;
	}
	return old;
}

void User::attack(EntityId id) {
	_lastAction = _time;
}

void User::disconnect() {
	Log::trace("disconnect user");
}

void User::reconnect() {
	Log::trace("reconnect user");
	_attribs.markAsDirty();
	visitVisible([&] (const EntityPtr& e) {
		sendEntitySpawn(e);
	});
}

bool User::update(long dt) {
	_time += dt;
	if (!Entity::update(dt)) {
		return false;
	}

	if (_time - _lastAction > _userTimeout->ulongVal()) {
		disconnect();
		return false;
	}

	// invalid id means spectator
	if (id() == -1) {
		// TODO: if a certain time has passed, get another point of interest
		//const glm::vec3& poi = _poiProvider->getPointOfInterest();
		//_pos = poi;
		_lastAction = _time;
		return true;
	}

	if (!isMove(network::MoveDirection::ANY)) {
		return true;
	}

	_lastAction = _time;

	glm::vec3 moveDelta = glm::vec3(0.0f);
	const float speed = current(attrib::Type::SPEED) * static_cast<float>(dt) / 1000.0f;
	if (isMove(network::MoveDirection::MOVELEFT)) {
		moveDelta += glm::left * speed;
	} else if (isMove(network::MoveDirection::MOVERIGHT)) {
		moveDelta += glm::right * speed;
	}
	if (isMove(network::MoveDirection::MOVEFORWARD)) {
		moveDelta += glm::forward * speed;
	} else if (isMove(network::MoveDirection::MOVEBACKWARD)) {
		moveDelta += glm::backward * speed;
	}

	_pos += glm::quat(glm::vec3(orientation(), _yaw, 0.0f)) * moveDelta;
	// TODO: if not flying...
	_pos.y = _world->findFloor(_pos.x, _pos.z, voxel::isFloor);
	Log::trace("move: dt %li, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", dt, speed, _pos.x, _pos.y, _pos.z, orientation(), _yaw);

	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	_messageSender->sendServerMessage(_peer, _entityUpdateFbb,
			network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFbb, id(), &pos, orientation()).Union());

	return true;
}

void User::sendSeed(long seed) const {
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::Seed, network::CreateSeed(fbb, seed).Union());
}

void User::sendUserSpawn() const {
	flatbuffers::FlatBufferBuilder fbb;
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	// TODO: broadcast to visible
	_messageSender->broadcastServerMessage(fbb, network::ServerMsgType::UserSpawn, network::CreateUserSpawn(fbb, id(), fbb.CreateString(_name), &pos).Union());
}

}
