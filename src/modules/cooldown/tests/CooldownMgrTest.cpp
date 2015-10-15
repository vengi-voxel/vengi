#include <gtest/gtest.h>
#include "cooldown/CooldownMgr.h"

namespace cooldown {

namespace {
const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
}

TEST(CooldownMgr, testTriggerCooldown) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown couldn't get triggered";
}

TEST(CooldownMgr, testCancelCooldown) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_TRUE(mgr.cancelCooldown(CooldownType::LOGOUT)) << "Failed to cancel the logout cooldown";
}

TEST(CooldownMgr, testExpireCooldown) {
	CooldownMgr mgr(timeProvider);
	timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(mgr.defaultDuration(CooldownType::LOGOUT), mgr.cooldown(CooldownType::LOGOUT)->durationMillis());
	ASSERT_EQ(mgr.defaultDuration(CooldownType::LOGOUT), mgr.cooldown(CooldownType::LOGOUT)->duration());
	ASSERT_TRUE(mgr.cooldown(CooldownType::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(mgr.cooldown(CooldownType::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(mgr.isCooldown(CooldownType::LOGOUT));
	mgr.update();
	ASSERT_TRUE(mgr.cooldown(CooldownType::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(mgr.cooldown(CooldownType::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(mgr.isCooldown(CooldownType::LOGOUT));
	timeProvider->update(mgr.defaultDuration(CooldownType::LOGOUT));
	ASSERT_FALSE(mgr.isCooldown(CooldownType::LOGOUT));
	mgr.update();
	ASSERT_FALSE(mgr.cooldown(CooldownType::LOGOUT)->running()) << "Cooldown is still running";
	ASSERT_FALSE(mgr.isCooldown(CooldownType::LOGOUT));
	ASSERT_TRUE(mgr.resetCooldown(CooldownType::LOGOUT)) << "Failed to reset the logout cooldown";
}

TEST(CooldownMgr, testMultipleCooldown) {
	CooldownMgr mgr(timeProvider);

	timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::INCREASE)) << "Increase cooldown couldn't get triggered";
	ASSERT_TRUE(mgr.isCooldown(CooldownType::LOGOUT));
	ASSERT_TRUE(mgr.isCooldown(CooldownType::INCREASE));
	mgr.update();

	const unsigned long logoutDuration = mgr.defaultDuration(CooldownType::LOGOUT);
	const unsigned long increaseDuration = mgr.defaultDuration(CooldownType::INCREASE);

	if (logoutDuration > increaseDuration) {
		timeProvider->update(increaseDuration);
		mgr.update();
		ASSERT_TRUE(mgr.isCooldown(CooldownType::LOGOUT));
		ASSERT_FALSE(mgr.isCooldown(CooldownType::INCREASE));
	} else {
		timeProvider->update(logoutDuration);
		mgr.update();
		ASSERT_TRUE(mgr.isCooldown(CooldownType::INCREASE));
		ASSERT_FALSE(mgr.isCooldown(CooldownType::LOGOUT));
	}
}

TEST(CooldownMgr, testTriggerCooldownTwice) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::ALREADY_RUNNING, mgr.triggerCooldown(CooldownType::LOGOUT)) << "Logout cooldown was triggered twice";
}

}
