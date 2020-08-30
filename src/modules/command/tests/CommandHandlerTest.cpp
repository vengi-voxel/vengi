/**
 * @file
 */

#include <gtest/gtest.h>
#include "command/CommandHandler.h"
#include "core/Var.h"

namespace command {

class CommandHandlerTest: public testing::Test {
public:
	void TearDown() override {
		core::Var::shutdown();
	}
};

TEST_F(CommandHandlerTest, testReplacePlaceholdersSmallBuffer) {
	core::Var::get("somename", "somevalue");
	char buf[16];
	ASSERT_FALSE(command::replacePlaceholders("foobar <cvar:somename>", buf, sizeof(buf)));
}

TEST_F(CommandHandlerTest, testReplacePlaceholdersPerfectFit) {
	core::Var::get("somename", "somevalue");
	char buf[17];
	ASSERT_TRUE(command::replacePlaceholders("foobar <cvar:somename>", buf, sizeof(buf)));
	ASSERT_STREQ("foobar somevalue", buf);
}

TEST_F(CommandHandlerTest, testExecuteCommandline) {
	EXPECT_EQ(-1, command::executeCommands("test"));
	EXPECT_EQ(-1, command::executeCommands("test/*foo*/"));
	EXPECT_EQ(-1, command::executeCommands("test;test;test;;"));
	EXPECT_EQ(0, command::executeCommands(""));
	EXPECT_EQ(0, command::executeCommands(";;;"));
	EXPECT_EQ(0, command::executeCommands("; ; ; "));
}

}
