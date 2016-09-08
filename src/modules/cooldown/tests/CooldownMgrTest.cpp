/**
 * @file
 */

#include <gtest/gtest.h>
#include "cooldown/CooldownMgr.h"

namespace cooldown {

namespace {
const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
}

TEST(CooldownMgr, testTriggerCooldown) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
}

TEST(CooldownMgr, testCancelCooldown) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_TRUE(mgr.cancelCooldown(Type::LOGOUT)) << "Failed to cancel the logout cooldown";
}

TEST(CooldownMgr, testExpireCooldown) {
	CooldownMgr mgr(timeProvider);
	timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(mgr.defaultDuration(Type::LOGOUT), mgr.cooldown(Type::LOGOUT)->durationMillis());
	ASSERT_EQ(mgr.defaultDuration(Type::LOGOUT), mgr.cooldown(Type::LOGOUT)->duration());
	ASSERT_TRUE(mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(mgr.isCooldown(Type::LOGOUT));
	mgr.update();
	ASSERT_TRUE(mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	ASSERT_TRUE(mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	ASSERT_TRUE(mgr.isCooldown(Type::LOGOUT));
	timeProvider->update(mgr.defaultDuration(Type::LOGOUT));
	ASSERT_FALSE(mgr.isCooldown(Type::LOGOUT));
	mgr.update();
	ASSERT_FALSE(mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is still running";
	ASSERT_FALSE(mgr.isCooldown(Type::LOGOUT));
	ASSERT_TRUE(mgr.resetCooldown(Type::LOGOUT)) << "Failed to reset the logout cooldown";
}

TEST(CooldownMgr, testMultipleCooldown) {
	CooldownMgr mgr(timeProvider);

	timeProvider->update(0ul);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::INCREASE)) << "Increase cooldown couldn't get triggered";
	ASSERT_TRUE(mgr.isCooldown(Type::LOGOUT));
	ASSERT_TRUE(mgr.isCooldown(Type::INCREASE));
	mgr.update();

	const unsigned long logoutDuration = mgr.defaultDuration(Type::LOGOUT);
	const unsigned long increaseDuration = mgr.defaultDuration(Type::INCREASE);

	if (logoutDuration > increaseDuration) {
		timeProvider->update(increaseDuration);
		mgr.update();
		ASSERT_TRUE(mgr.isCooldown(Type::LOGOUT));
		ASSERT_FALSE(mgr.isCooldown(Type::INCREASE));
	} else {
		timeProvider->update(logoutDuration);
		mgr.update();
		ASSERT_TRUE(mgr.isCooldown(Type::INCREASE));
		ASSERT_FALSE(mgr.isCooldown(Type::LOGOUT));
	}
}

TEST(CooldownMgr, testTriggerCooldownTwice) {
	CooldownMgr mgr(timeProvider);
	ASSERT_EQ(CooldownTriggerState::SUCCESS, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	ASSERT_EQ(CooldownTriggerState::ALREADY_RUNNING, mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown was triggered twice";
}

}
