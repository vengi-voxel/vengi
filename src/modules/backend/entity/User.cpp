/**
 * @file
 */

#include "User.h"
#include "core/Var.h"
#include "voxel/World.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id, const std::string& name, const network::MessageSenderPtr& messageSender,
		const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const PoiProviderPtr& poiProvider) :
		Entity(id, messageSender, timeProvider, containerProvider), _peer(nullptr),
		_name(name), _world(world), _poiProvider(poiProvider), _pitch(0.0f), _yaw(0.0f),
		_lastAction(0ul), _time(0ul) {
	setPeer(peer);
	const glm::vec3& poi = _poiProvider->getPointOfInterest();
	_pos = poi;
	_attribs.setCurrent(attrib::Type::SPEED, 60.0);
	_attribs.setCurrent(attrib::Type::VIEWDISTANCE, 500.0);
	_userTimeout = core::Var::get(cfg::ServerUserTimeout, "60000");
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

void User::setPos(const glm::vec3& pos) {
	const int y = _world->findFloor(pos.x, pos.z, voxel::isFloor);
	_pos = pos;
	if (_pos.y < y) {
		_pos.y = y;
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
	_attribs.markAsDirty();
	Log::trace("reconnect user");
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

	_pos += glm::quat(glm::vec3(_pitch, _yaw, 0.0f)) * moveDelta;
	// TODO: if not flying...
	_pos.y = _world->findFloor(_pos.x, _pos.z, voxel::isFloor);
	Log::trace("move: dt %li, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", dt, speed, _pos.x, _pos.y, _pos.z, _pitch, _yaw);

	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	_messageSender->sendServerMessage(_peer, _entityUpdateFbb,
			network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFbb, id(), &pos, orientation()).Union());

	return true;
}

void User::sendSeed(long seed) const {
	static flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::Seed, network::CreateSeed(fbb, seed).Union());
}

void User::sendUserSpawn() const {
	static flatbuffers::FlatBufferBuilder fbb;
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	// TODO: broadcast to visible
	_messageSender->broadcastServerMessage(fbb, network::ServerMsgType::UserSpawn, network::CreateUserSpawn(fbb, id(), fbb.CreateString(_name), &pos).Union());
}

void User::sendEntityUpdate(const EntityPtr& entity) const {
	static flatbuffers::FlatBufferBuilder fbb;
	const glm::vec3& _pos = entity->pos();
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::EntityUpdate, network::CreateEntityUpdate(fbb, entity->id(), &pos, entity->orientation()).Union());
}

void User::sendEntitySpawn(const EntityPtr& entity) const {
	static flatbuffers::FlatBufferBuilder fbb;
	const glm::vec3& pos = entity->pos();
	const network::Vec3 vec3 { pos.x, pos.y, pos.z };
	const EntityId entityId = id();
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::EntitySpawn, network::CreateEntitySpawn(fbb, entity->id(), entity->npcType(), &vec3, entityId).Union());
}

void User::sendEntityRemove(const EntityPtr& entity) const {
	static flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::EntityRemove, network::CreateEntityRemove(fbb, entity->id()).Union());
}

}
