/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "shared/SharedMovement.h"

namespace backend {

class User;

class UserMovementMgr : public core::IComponent {
private:
	shared::SharedMovement _movement;
	User* _user;
	flatbuffers::FlatBufferBuilder _entityUpdateFBB;
	bool _sendUpdate = false;
public:
	UserMovementMgr(User* user);

	void changeMovement(network::MoveDirection bitmask, float pitch, float yaw);
	void update(long dt);
	bool init() override;
	void shutdown() override;
};

}
