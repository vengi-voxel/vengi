/**
 * @file
 */

#include "UserCooldownMgr.h"
#include "CooldownModel.h"
#include "core/Log.h"

namespace backend {

UserCooldownMgr::UserCooldownMgr(EntityId userId,
		const core::TimeProviderPtr& timeProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::DBHandlerPtr& dbHandler) :
		Super(timeProvider, cooldownProvider), _dbHandler(dbHandler), _userId(userId) {
}

void UserCooldownMgr::init() {
	if (!_dbHandler->select(db::CooldownModel(), db::DBConditionCooldownUserid(_userId), [this] (db::CooldownModel&& model) {
		const cooldown::Type type = (cooldown::Type)model.cooldownid();
		const uint64_t millis = model.starttime().millis();
		// TODO: load cooldowns
	})) {
		Log::warn("Could not load cooldowns for user %" PRIEntId, _userId);
	}
}

void UserCooldownMgr::shutdown() {
	core::ScopedWriteLock lock(_lock);
	for (const auto& e : _cooldowns) {
		const cooldown::CooldownPtr& c = e.second;
		db::CooldownModel model;
		model.setCooldownid(c->type());
		model.setUserid(_userId);
		model.setStarttime(c->startMillis());
		_dbHandler->insert(model);
	}
}

}
