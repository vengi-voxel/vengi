/**
 * @file
 */

#include "UserLogoutMgr.h"
#include "UserCooldownMgr.h"

namespace backend {

UserLogoutMgr::UserLogoutMgr(UserCooldownMgr& cooldownMgr) :
		_cooldownMgr(cooldownMgr) {
}

void UserLogoutMgr::triggerLogout() {
	_cooldownMgr.triggerCooldown(cooldown::Type::LOGOUT, [&] (cooldown::CallbackType callbackType) {
		if (callbackType == cooldown::CallbackType::Expired) {
			_disconnect = true;
		}
	});
}

void UserLogoutMgr::update(long dt) {
}

bool UserLogoutMgr::init() {
	return true;
}

void UserLogoutMgr::shutdown() {
}

}
