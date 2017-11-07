/**
 * @file
 */

#include "User.h"
#include "core/Var.h"
#include "backend/world/World.h"

namespace backend {

User::User(ENetPeer* peer, EntityId id, const std::string& name, const MapPtr& map, const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider, const persistence::DBHandlerPtr& dbHandler,
		const stock::StockProviderPtr& stockDataProvider) :
		Super(id, map, messageSender, timeProvider, containerProvider, cooldownProvider),
		_name(name), _dbHandler(dbHandler), _stockMgr(this, stockDataProvider, dbHandler),
		_timeProvider(timeProvider), _cooldownProvider(cooldownProvider) {
	setPeer(peer);
	_entityType = network::EntityType::PLAYER;
	_userTimeout = core::Var::getSafe(cfg::ServerUserTimeout);
}

void User::init() {
	Super::init();
	_stockMgr.init();
}

void User::visibleAdd(const EntitySet& entities) {
	Super::visibleAdd(entities);
	for (const EntityPtr& e : entities) {
		sendEntitySpawn(e);
	}
}

void User::visibleRemove(const EntitySet& entities) {
	Super::visibleRemove(entities);
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

void User::onCooldownExpired(cooldown::Type type) {
	if (type == cooldown::Type::LOGOUT) {
		_disconnect = true;
	}
}

void User::triggerLogout() {
	triggerCooldown(cooldown::Type::LOGOUT);
}

void User::triggerCooldown(cooldown::Type type) {
	_cooldowns.triggerCooldown(type, [this, type] (cooldown::CallbackType callbackType) {
		if (callbackType == cooldown::CallbackType::Expired) {
			onCooldownExpired(type);
			return;
		}
		sendCooldown(type, callbackType == cooldown::CallbackType::Started);
	});
}

void User::reconnect() {
	Log::trace("reconnect user");
	_attribs.markAsDirty();
	visitVisible([&] (const EntityPtr& e) {
		sendEntitySpawn(e);
	});
}

bool User::update(long dt) {
	if (_disconnect) {
		return false;
	}
	_time += dt;
	if (!Super::update(dt)) {
		return false;
	}

	if (_time - _lastAction > _userTimeout->ulongVal()) {
		triggerLogout();
		return true;
	}

	_stockMgr.update(dt);
	_cooldowns.update();

	if (!isMove(network::MoveDirection::ANY)) {
		return true;
	}

	_lastAction = _time;

	glm::vec3 moveDelta {0.0f};
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
	_pos.y = _map->findFloor(_pos);
	Log::trace("move: dt %li, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", dt, speed, _pos.x, _pos.y, _pos.z, orientation(), _yaw);

	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	// TODO: broadcast to visible
	_messageSender->broadcastServerMessage(_entityUpdateFbb,
			network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFbb, id(), &pos, orientation()).Union());

	return true;
}

void User::sendSeed(long seed) const {
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendServerMessage(_peer, fbb, network::ServerMsgType::Seed, network::CreateSeed(fbb, seed).Union());
}

void User::sendCooldown(cooldown::Type type, bool started) const {
	flatbuffers::FlatBufferBuilder fbb;
	network::ServerMsgType msgtype;
	flatbuffers::Offset<void> msg;
	if (started) {
		const uint64_t duration = _cooldownProvider->duration(type);
		const uint64_t now = _timeProvider->tickMillis();
		msg = network::CreateStartCooldown(fbb, type, now, duration).Union();
		msgtype = network::ServerMsgType::StartCooldown;
	} else {
		msg = network::CreateStopCooldown(fbb, type).Union();
		msgtype = network::ServerMsgType::StopCooldown;
	}

	_messageSender->sendServerMessage(_peer, fbb, msgtype, msg);
}

void User::sendUserSpawn() const {
	flatbuffers::FlatBufferBuilder fbb;
	const network::Vec3 pos { _pos.x, _pos.y, _pos.z };
	// TODO: broadcast to visible
	_messageSender->broadcastServerMessage(fbb, network::ServerMsgType::UserSpawn, network::CreateUserSpawn(fbb, id(), fbb.CreateString(_name), &pos).Union());
}

}
