/**
 * @file
 */

#include "User.h"
#include "util/Velocity.h"
#include "core/Var.h"
#include "voxel/World.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id, const std::string& name, const network::MessageSenderPtr& messageSender,
		const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const PoiProviderPtr& poiProvider) :
		Entity(id, messageSender, timeProvider, containerProvider), _peer(nullptr), _name(name), _world(world), _poiProvider(poiProvider), _moveMask(
				0u), _pitch(0.0f), _yaw(0.0f), _lastAction(0ul), _time(0ul) {
	setPeer(peer);
	const glm::vec3& poi = _poiProvider->getPointOfInterest();
	_pos = poi;
	_attribs.setCurrent(attrib::Types::SPEED, 60.0);
	_attribs.setCurrent(attrib::Types::VIEWDISTANCE, 500.0);
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

	if (!isMove(MoveDirection::ANY)) {
		return true;
	}

	_lastAction = _time;

	const float speedVal = current(attrib::Types::SPEED);
	const float deltaTime = static_cast<float>(dt) / 1000.0f;
	if (isMove(MoveDirection::MOVEFORWARD | MoveDirection::MOVEBACKWARD)) {
		const glm::vec3& direction = util::getDirection(_pitch, _yaw);
		const glm::vec3 velocity(direction * speedVal);
		if (isMove(MoveDirection::MOVEFORWARD)) {
			setPos(pos() + velocity * deltaTime);
			Log::trace("move forward: dt %f, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", deltaTime, speedVal, _pos.x, _pos.y, _pos.z, _pitch, _yaw);
		} else if (isMove(MoveDirection::MOVEBACKWARD)) {
			setPos(pos() - velocity * deltaTime);
			Log::trace("move backward: dt %f, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", deltaTime, speedVal, _pos.x, _pos.y, _pos.z, _pitch, _yaw);
		}
	}
	if (isMove(MoveDirection::MOVERIGHT | MoveDirection::MOVELEFT)) {
		const glm::vec3& direction = util::getDirection(_yaw);
		const glm::vec3 velocity(direction * speedVal);
		if (isMove(MoveDirection::MOVELEFT)) {
			setPos(pos() - velocity * deltaTime);
			Log::trace("move left: dt %f, speed: %f p(%f:%f:%f)", deltaTime, speedVal, _pos.x, _pos.y, _pos.z);
		} else if (isMove(MoveDirection::MOVERIGHT)) {
			setPos(pos() + velocity * deltaTime);
			Log::trace("move right: dt %f, speed: %f p(%f:%f:%f)", deltaTime, speedVal, _pos.x, _pos.y, _pos.z);
		}
	}

	flatbuffers::FlatBufferBuilder fbb;
	const network::messages::Vec3 pos { _pos.x, _pos.y, _pos.z };
	sendServerMsg(EntityUpdate(fbb, id(), &pos, orientation()), EntityUpdate);

	return true;
}

void User::sendSeed(long seed) const {
	flatbuffers::FlatBufferBuilder fbb;
	sendServerMsg(Seed(fbb, seed), Seed);
}

void User::sendUserSpawn() const {
	flatbuffers::FlatBufferBuilder fbb;
	const network::messages::Vec3 pos { _pos.x, _pos.y, _pos.z };
	broadcastServerMsg(UserSpawn(fbb, id(), fbb.CreateString(_name), &pos), UserSpawn);
}

void User::sendEntityUpdate(const EntityPtr& entity) const {
	flatbuffers::FlatBufferBuilder fbb;
	const glm::vec3& _pos = entity->pos();
	const network::messages::Vec3 pos { _pos.x, _pos.y, _pos.z };
	sendServerMsg(EntityUpdate(fbb, entity->id(), &pos, entity->orientation()), EntityUpdate);
}

void User::sendEntitySpawn(const EntityPtr& entity) const {
	flatbuffers::FlatBufferBuilder fbb;
	const glm::vec3& pos = entity->pos();
	const network::messages::Vec3 vec3 { pos.x, pos.y, pos.z };
	const long humanId = id(); // -1;
	sendServerMsg(NpcSpawn(fbb, entity->id(), entity->npcType(), &vec3, humanId), NpcSpawn);
}

void User::sendEntityRemove(const EntityPtr& entity) const {
	flatbuffers::FlatBufferBuilder fbb;
	sendServerMsg(EntityRemove(fbb, entity->id()), EntityRemove);
}

}
