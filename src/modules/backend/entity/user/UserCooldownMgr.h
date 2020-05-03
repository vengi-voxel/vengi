/**
 * @file
 */

#pragma once

#include "cooldown/CooldownMgr.h"
#include "persistence/ForwardDecl.h"
#include "persistence/ISavable.h"
#include "core/FourCC.h"
#include "backend/entity/EntityId.h"
#include "CooldownModel.h"
#include <vector>

namespace backend {

class User;

/**
 * @brief The UserCooldownMgr is responsible for persisting and sending out cooldown states
 * @ingroup Cooldowns
 */
class UserCooldownMgr : public cooldown::CooldownMgr, public persistence::ISavable {
private:
	static constexpr uint32_t FOURCC = FourCC('C','O','O','L');
	using Super = cooldown::CooldownMgr;
	persistence::DBHandlerPtr _dbHandler;
	persistence::PersistenceMgrPtr _persistenceMgr;
	User* _user;
	mutable flatbuffers::FlatBufferBuilder _cooldownFBB;
	std::vector<db::CooldownModel> _dirtyModels;
public:
	UserCooldownMgr(User* user,
			const core::TimeProviderPtr& timeProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider,
			const persistence::DBHandlerPtr& dbHandler,
			const persistence::PersistenceMgrPtr& persistenceMgr);

	bool init() override;
	void shutdown() override;

	cooldown::CooldownTriggerState triggerCooldown(cooldown::Type type, const cooldown::CooldownCallback& callback = cooldown::CooldownCallback()) override;
	void sendCooldown(cooldown::Type type, bool started) const;

	bool getDirtyModels(Models& models) override;
};

}
