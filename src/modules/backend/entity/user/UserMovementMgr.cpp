/**
 * @file
 */

#include "UserMovementMgr.h"
#include "backend/entity/User.h"
#include "backend/world/Map.h"

namespace backend {

UserMovementMgr::UserMovementMgr(User* user) : _user(user) {
}

void UserMovementMgr::changeMovement(network::MoveDirection bitmask, float pitch, float yaw) {
	_moveMask = bitmask;
	_user->setOrientation(pitch);
	_yaw = yaw;
}

void UserMovementMgr::update(long dt) {
	if (!isMove(network::MoveDirection::ANY)) {
		return;
	}

	glm::vec3 moveDelta {0.0f};
	const float speed = _user->current(attrib::Type::SPEED) * static_cast<float>(dt) / 1000.0f;
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

	const MapPtr& map = _user->map();
	const float orientation = _user->orientation();
	glm::vec3 pos = _user->pos();

	pos += glm::quat(glm::vec3(orientation, _yaw, 0.0f)) * moveDelta;
	// TODO: if not flying...
	pos.y = map->findFloor(pos);
	Log::trace("move: dt %li, speed: %f p(%f:%f:%f), pitch: %f, yaw: %f", dt, speed,
			pos.x, pos.y, pos.z, orientation, _yaw);
	_user->setPos(pos);

	const network::Vec3 netPos { pos.x, pos.y, pos.z };
	_user->sendToVisible(_entityUpdateFBB,
			network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFBB, _user->id(), &netPos, orientation).Union(), true);

	_user->logoutMgr().updateLastActionTime();
}

bool UserMovementMgr::init() {
	return true;
}

void UserMovementMgr::shutdown() {
	const EntityId userId = _user->id();
	Log::info("Shutdown movement manager for user " PRIEntId, userId);
}

}
