/**
 * @file
 */

#pragma once

#include "cooldown/CooldownMgr.h"
#include "persistence/DBHandler.h"
#include "backend/entity/EntityId.h"

namespace backend {

class UserCooldownMgr : public cooldown::CooldownMgr {
private:
	using Super = cooldown::CooldownMgr;
	persistence::DBHandlerPtr _dbHandler;
	EntityId _userId;
public:
	UserCooldownMgr(EntityId userId,
			const core::TimeProviderPtr& timeProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler);

	void init() override;

	void shutdown() override;
};

}
