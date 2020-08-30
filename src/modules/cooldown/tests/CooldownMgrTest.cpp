/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "cooldown/CooldownMgr.h"

#include "../CooldownProvider.h"
#include "core/Singleton.h"
#include "io/Filesystem.h"

namespace cooldown {

class CooldownMgrTest : public core::AbstractTest {
protected:
	core::TimeProviderPtr _timeProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	CooldownMgr _mgr;
public:
	CooldownMgrTest() :
		_timeProvider(std::make_shared<core::TimeProvider>()),
		_cooldownProvider(std::make_shared<cooldown::CooldownProvider>()),
		_mgr(_timeProvider, _cooldownProvider) {
	}

	void SetUp() override {
		core::AbstractTest::SetUp();
		const core::String& cooldowns = io::filesystem()->load("cooldowns.lua");
		_cooldownProvider->init(cooldowns);
	}
};

TEST_F(CooldownMgrTest, testTriggerCooldown) {
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
}

TEST_F(CooldownMgrTest, testCancelCooldown) {
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	EXPECT_TRUE(_mgr.cancelCooldown(Type::LOGOUT)) << "Failed to cancel the logout cooldown";
}

TEST_F(CooldownMgrTest, testExpireCooldown) {
	_timeProvider->setTickTime(0ul);
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	EXPECT_EQ(_mgr.defaultDuration(Type::LOGOUT), _mgr.cooldown(Type::LOGOUT)->durationMillis());
	EXPECT_EQ(_mgr.defaultDuration(Type::LOGOUT), _mgr.cooldown(Type::LOGOUT)->duration());
	EXPECT_TRUE(_mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	EXPECT_TRUE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	EXPECT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	_mgr.update();
	EXPECT_TRUE(_mgr.cooldown(Type::LOGOUT)->started()) << "Cooldown is not started";
	EXPECT_TRUE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is not running";
	EXPECT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	_timeProvider->setTickTime(_mgr.defaultDuration(Type::LOGOUT));
	_mgr.update();
	EXPECT_FALSE(_mgr.cooldown(Type::LOGOUT)->running()) << "Cooldown is still running";
	EXPECT_FALSE(_mgr.isCooldown(Type::LOGOUT));
	EXPECT_TRUE(_mgr.resetCooldown(Type::LOGOUT)) << "Failed to reset the logout cooldown";
}

TEST_F(CooldownMgrTest, testMultipleCooldown) {
	_timeProvider->setTickTime(0ul);
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::INCREASE)) << "Increase cooldown couldn't get triggered";
	EXPECT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	EXPECT_TRUE(_mgr.isCooldown(Type::INCREASE));
	_mgr.update();
	EXPECT_TRUE(_mgr.isCooldown(Type::LOGOUT));
	EXPECT_TRUE(_mgr.isCooldown(Type::INCREASE));

	const unsigned long logoutDuration = _mgr.defaultDuration(Type::LOGOUT);
	const unsigned long increaseDuration = _mgr.defaultDuration(Type::INCREASE);

	if (logoutDuration > increaseDuration) {
		_timeProvider->setTickTime(increaseDuration);
		_mgr.update();
		EXPECT_TRUE(_mgr.isCooldown(Type::LOGOUT));
		EXPECT_FALSE(_mgr.isCooldown(Type::INCREASE));
	} else {
		_timeProvider->setTickTime(logoutDuration);
		_mgr.update();
		EXPECT_TRUE(_mgr.isCooldown(Type::INCREASE));
		EXPECT_FALSE(_mgr.isCooldown(Type::LOGOUT));
	}
}

TEST_F(CooldownMgrTest, testTriggerCooldownTwice) {
	EXPECT_EQ(CooldownTriggerState::SUCCESS, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown couldn't get triggered";
	EXPECT_EQ(CooldownTriggerState::ALREADY_RUNNING, _mgr.triggerCooldown(Type::LOGOUT)) << "Logout cooldown was triggered twice";
}

}
