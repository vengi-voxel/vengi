/**
 * @file
 */

#include "UserTest.h"

namespace backend {

class UserCooldownMgrTest : public UserTest {
private:
	using Super = UserTest;
protected:
	void SetUp() override {
		Super::SetUp();
		if (_dbSupported) {
			dbHandler->createOrUpdateTable(db::InventoryModel());
			dbHandler->createOrUpdateTable(db::CooldownModel());
			dbHandler->createOrUpdateTable(db::AttribModel());
		}
	}
};

TEST_F(UserCooldownMgrTest, testTriggerAndAbort) {
	const UserPtr& user = create(EntityId(1), "cooldown");
	UserCooldownMgr& mgr = user->cooldownMgr();
	ASSERT_EQ(cooldown::CooldownTriggerState::SUCCESS, mgr.triggerCooldown(cooldown::Type::INCREASE));
	ASSERT_EQ(cooldown::CooldownTriggerState::ALREADY_RUNNING, mgr.triggerCooldown(cooldown::Type::INCREASE));
	ASSERT_TRUE(mgr.cancelCooldown(cooldown::Type::INCREASE));
	shutdown(user);
}

}
