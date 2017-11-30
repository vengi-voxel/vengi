/**
 * @file
 */

#pragma once

namespace backend {

class UserCooldownMgr;

class UserLogoutMgr {
private:
	UserCooldownMgr& _cooldownMgr;
	bool _disconnect = false;
public:
	UserLogoutMgr(UserCooldownMgr& cooldownMgr);

	/**
	 * @brief The client wants to disconnect - the user object itself will stay in the server until
	 * a logout cooldown was hit
	 */
	void triggerLogout();
	bool isDisconnect() const;

	void update(long dt);
	bool init();
	void shutdown();
};

inline bool UserLogoutMgr::isDisconnect() const {
	return _disconnect;
}

}
