/**
 * @file
 */

#include <gtest/gtest.h>
#include "command/ActionButton.h"

namespace command {

class ActionButtonTest : public testing::Test {};

TEST_F(ActionButtonTest, testExecuteTogglePressAndRelease) {
	ActionButton button;
	int toggles = 0;

	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(0, toggles);

	EXPECT_TRUE(button.handleDown(1, 1.0));
	EXPECT_TRUE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(1, toggles);

	// held - no further toggles
	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(1, toggles);

	EXPECT_TRUE(button.handleUp(1, 2.0));
	EXPECT_TRUE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(2, toggles);

	// released - no further toggles
	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(2, toggles);
}

TEST_F(ActionButtonTest, testExecuteToggleIgnoresExtraKeysWhileHeld) {
	ActionButton button;
	int toggles = 0;

	EXPECT_TRUE(button.handleDown(1, 1.0));
	EXPECT_TRUE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(1, toggles);

	EXPECT_TRUE(button.handleDown(2, 1.5));
	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(1, toggles);

	EXPECT_FALSE(button.handleUp(1, 2.0));
	EXPECT_FALSE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(1, toggles);

	EXPECT_TRUE(button.handleUp(2, 2.5));
	EXPECT_TRUE(button.executeToggle([&]() { ++toggles; }));
	EXPECT_EQ(2, toggles);
}

} // namespace command
