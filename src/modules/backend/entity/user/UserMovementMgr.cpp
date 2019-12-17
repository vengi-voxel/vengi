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
	_movement.setMoveMask(bitmask);
	_user->setOrientation(yaw);
}

void UserMovementMgr::update(long dt) {
	const float speed = _user->current(attrib::Type::SPEED);
	const float deltaSeconds = static_cast<float>(dt) / 1000.0f;
	const float orientation = _user->orientation();
	const MapPtr& map = _user->map();
	const glm::vec3& newPos = _movement.update(deltaSeconds, orientation, speed, _user->pos(), [&] (const glm::vec3& pos, float maxWalkHeight) {
		return map->findFloor(pos, maxWalkHeight);
	});
	_user->setPos(newPos);
	_user->setAnimation(_movement.animation());

	const network::Vec3 netPos { newPos.x, newPos.y, newPos.z };
	_user->sendToVisible(_entityUpdateFBB,
			network::ServerMsgType::EntityUpdate,
			network::CreateEntityUpdate(_entityUpdateFBB, _user->id(), &netPos, orientation, _movement.animation()).Union(), true);

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
