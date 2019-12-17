/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include "core/IComponent.h"

namespace backend {

class UserCooldownMgr;

/**
 * @see UserConnectHandler
 */
class UserLogoutMgr : public core::IComponent {
private:
	UserCooldownMgr& _cooldownMgr;
	bool _disconnect = false;
	uint64_t _lastAction = 0u;
	uint64_t _time = 0u;
	core::VarPtr _userTimeout;

public:
	UserLogoutMgr(UserCooldownMgr& cooldownMgr);

	/**
	 * @brief The client wants to disconnect - the user object itself will stay in the server until
	 * a logout cooldown was hit
	 */
	void triggerLogout();
	bool isDisconnect() const;
	/**
	 * @brief Updating the last action time whenever the user did an action to prevent the automatic
	 * logout-on-inactivity to be triggered.
	 */
	void updateLastActionTime();

	void update(long dt);
	bool init() override;
	void shutdown() override;
};

inline bool UserLogoutMgr::isDisconnect() const {
	return _disconnect;
}

}
