/**
 * @file
 */

#include "UserMovementMgr.h"
#include "backend/entity/User.h"
#include "backend/world/Map.h"
#include "core/Trace.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace backend {

UserMovementMgr::UserMovementMgr(User* user) : _user(user) {
}

void UserMovementMgr::changeMovement(network::MoveDirection bitmask, float pitch, float yaw) {
	_sendUpdate |= _movement.moveMask() != bitmask || !glm::epsilonEqual(_user->orientation(), yaw, glm::epsilon<float>());
	_movement.setMoveMask(bitmask);
	_user->setOrientation(yaw);
}

void UserMovementMgr::update(long dt) {
	core_trace_scoped(UserMovementMgrUpdate);
	const float speed = _user->current(attrib::Type::SPEED);
	const float deltaSeconds = static_cast<float>(dt) / 1000.0f;
	const float orientation = _user->orientation();
	const MapPtr& map = _user->map();
	const glm::vec3 oldPos = _user->pos();
	const network::Animation oldAnimation = _user->animation();
	const glm::vec3& newPos = _movement.update(deltaSeconds, orientation, speed, oldPos, [&] (const glm::ivec3& pos, int maxWalkHeight) {
		return map->findFloor(pos, maxWalkHeight);
	});
	_user->setPos(newPos);
	_user->setAnimation(_movement.animation());

	if (_sendUpdate || _movement.animation() != oldAnimation || !glm::all(glm::epsilonEqual(oldPos, newPos, glm::epsilon<float>()))) {
		const network::Vec3 netPos { newPos.x, newPos.y, newPos.z };
		_user->sendToVisible(_entityUpdateFBB,
				network::ServerMsgType::EntityUpdate,
				network::CreateEntityUpdate(_entityUpdateFBB, _user->id(), &netPos, orientation, _movement.animation()).Union(), true, 0u);
		_sendUpdate = false;
	}

	if (_movement.moveMask() != network::MoveDirection::NONE) {
		_user->logoutMgr().updateLastActionTime();
	}
}

bool UserMovementMgr::init() {
	return true;
}

void UserMovementMgr::shutdown() {
	const EntityId userId = _user->id();
	Log::trace("Shutdown movement manager for user " PRIEntId, userId);
}

}
