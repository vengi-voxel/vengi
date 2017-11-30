/**
 * @file
 */

#pragma once

#include "cooldown/CooldownMgr.h"
#include "persistence/DBHandler.h"
#include "backend/entity/EntityId.h"
#include "network/ServerMessageSender.h"

namespace backend {

class User;

class UserCooldownMgr : public cooldown::CooldownMgr {
private:
	using Super = cooldown::CooldownMgr;
	persistence::DBHandlerPtr _dbHandler;
	User* _user;
	mutable flatbuffers::FlatBufferBuilder _cooldownFBB;
public:
	UserCooldownMgr(User* user,
			const core::TimeProviderPtr& timeProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler);

	void init() override;
	void shutdown() override;

	cooldown::CooldownTriggerState triggerCooldown(cooldown::Type type, cooldown::CooldownCallback callback = cooldown::CooldownCallback()) override;
	void sendCooldown(cooldown::Type type, bool started) const;
};

}
