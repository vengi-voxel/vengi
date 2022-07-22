/**
 * @file
 */

#include "command/CommandHandler.h"
#include "command/Command.h"
#include "core/Var.h"
#include <gtest/gtest.h>

namespace command {

class CommandHandlerTest : public testing::Test {
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

TEST_F(CommandHandlerTest, testExecuteCommandsQuoteWhitespace) {
	core::String parameter;
	command::Command::registerCommand("testquitewhitespace", [&](const command::CmdArgs &args) {
		if (args.size() == 1) {
			parameter = args[0];
		}
	});

	EXPECT_EQ(-1, command::executeCommands("testquitewhitespace \"foobar barfoo\";nocommand"));
	EXPECT_STREQ("foobar barfoo", parameter.c_str());
}

} // namespace command
