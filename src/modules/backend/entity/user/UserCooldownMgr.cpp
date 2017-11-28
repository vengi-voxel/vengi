/**
 * @file
 */

#include "UserCooldownMgr.h"
#include "CooldownModel.h"
#include "persistence/DBHandler.h"
#include "core/Log.h"
#include "network/ServerMessageSender.h"
#include "backend/entity/User.h"

namespace backend {

UserCooldownMgr::UserCooldownMgr(User* user,
		const core::TimeProviderPtr& timeProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider,
		const persistence::DBHandlerPtr& dbHandler,
		const network::ServerMessageSenderPtr& messageSender) :
		Super(timeProvider, cooldownProvider), _dbHandler(dbHandler), _messageSender(messageSender), _user(user) {
}

void UserCooldownMgr::init() {
	core::ScopedWriteLock lock(_lock);
	if (!_dbHandler->select(db::CooldownModel(), db::DBConditionCooldownModelUserid(_user->id()), [this] (db::CooldownModel&& model) {
		const int32_t id = model.cooldownid();
		const cooldown::Type type = (cooldown::Type)id;
		const uint64_t millis = model.starttime().millis();
		const cooldown::CooldownPtr& cooldown = createCooldown(type, millis);
		_cooldowns[type] = cooldown;
		_queue.push(cooldown);
	})) {
		Log::warn("Could not load cooldowns for user " PRIEntId, _user->id());
	}
}

void UserCooldownMgr::shutdown() {
	core::ScopedWriteLock lock(_lock);
	for (const auto& e : _cooldowns) {
		const cooldown::CooldownPtr& c = e.second;
		db::CooldownModel model;
		model.setCooldownid(c->type());
		model.setUserid(_user->id());
		model.setStarttime(c->startMillis());
		_dbHandler->insert(model);
	}
}

void UserCooldownMgr::onCooldownExpired(cooldown::Type type) {
	if (type == cooldown::Type::LOGOUT) {
		_user->_disconnect = true;
	}
}

cooldown::CooldownTriggerState UserCooldownMgr::triggerCooldown(cooldown::Type type, cooldown::CooldownCallback callback) {
	return Super::triggerCooldown(type, [this, type, callback] (cooldown::CallbackType callbackType) {
		callback(callbackType);
		if (callbackType == cooldown::CallbackType::Expired) {
			onCooldownExpired(type);
		}
		sendCooldown(type, callbackType == cooldown::CallbackType::Started);
	});
}

void UserCooldownMgr::sendCooldown(cooldown::Type type, bool started) const {
	network::ServerMsgType msgtype;
	flatbuffers::Offset<void> msg;
	if (started) {
		const uint64_t duration = _cooldownProvider->duration(type);
		const uint64_t now = _timeProvider->tickMillis();
		msg = network::CreateStartCooldown(_cooldownFBB, type, now, duration).Union();
		msgtype = network::ServerMsgType::StartCooldown;
	} else {
		msg = network::CreateStopCooldown(_cooldownFBB, type).Union();
		msgtype = network::ServerMsgType::StopCooldown;
	}
	_messageSender->sendServerMessage(_user->peer(), _cooldownFBB, msgtype, msg);
}


}
