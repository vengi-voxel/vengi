/**
 * @file
 */

#include "UserLogoutMgr.h"
#include "UserCooldownMgr.h"
#include "core/GameConfig.h"
#include "core/Var.h"
#include "core/Log.h"

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

void UserLogoutMgr::updateLastActionTime() {
	_lastAction = _time;
}

void UserLogoutMgr::update(long dt) {
	_time += dt;

	if (_time - _lastAction > _userTimeout->ulongVal()) {
		triggerLogout();
	}
}

bool UserLogoutMgr::init() {
	_userTimeout = core::Var::getSafe(cfg::ServerUserTimeout);
	return true;
}

void UserLogoutMgr::shutdown() {
	Log::info("Shutdown logout manager");
}

}
