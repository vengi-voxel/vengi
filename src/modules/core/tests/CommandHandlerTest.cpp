/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/command/CommandHandler.h"
#include "core/Var.h"

namespace core {

class CommandHandlerTest: public testing::Test {
public:
	void TearDown() override {
		core::Var::shutdown();
	}
};

TEST_F(CommandHandlerTest, testReplacePlaceholdersSmallBuffer) {
	core::Var::get("somename", "somevalue");
	char buf[16];
	ASSERT_FALSE(core::replacePlaceholders("foobar <cvar:somename>", buf, sizeof(buf)));
}

TEST_F(CommandHandlerTest, testReplacePlaceholdersPerfectFit) {
	core::Var::get("somename", "somevalue");
	char buf[17];
	ASSERT_TRUE(core::replacePlaceholders("foobar <cvar:somename>", buf, sizeof(buf)));
	ASSERT_STREQ("foobar somevalue", buf);
}

TEST_F(CommandHandlerTest, testExecuteCommandline) {
	EXPECT_EQ(-1, core::executeCommands("test"));
	EXPECT_EQ(-1, core::executeCommands("test/*foo*/"));
	EXPECT_EQ(-1, core::executeCommands("test;test;test;;"));
	EXPECT_EQ(0, core::executeCommands(""));
	EXPECT_EQ(0, core::executeCommands(";;;"));
	EXPECT_EQ(0, core::executeCommands("; ; ; "));
}

}
