/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "network/ServerMessageSender.h"

namespace backend {

class User;

class UserMovementMgr : public core::IComponent {
private:
	network::MoveDirection _moveMask = network::MoveDirection::NONE;
	float _yaw = 0.0f;
	User* _user;
	flatbuffers::FlatBufferBuilder _entityUpdateFBB;

	bool isMove(network::MoveDirection dir) const;
	void addMove(network::MoveDirection dir);
	void removeMove(network::MoveDirection dir);

public:
	UserMovementMgr(User* user);

	void changeMovement(network::MoveDirection bitmask, float pitch, float yaw);
	void update(long dt);
	bool init() override;
	void shutdown() override;
};

inline bool UserMovementMgr::isMove(network::MoveDirection dir) const {
	return (_moveMask & dir) != network::MoveDirection::NONE;
}

inline void UserMovementMgr::addMove(network::MoveDirection dir) {
	_moveMask |= dir;
}

inline void UserMovementMgr::removeMove(network::MoveDirection dir) {
	_moveMask &= ~dir;
}

}
