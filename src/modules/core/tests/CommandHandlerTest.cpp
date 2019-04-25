/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/command/CommandHandler.h"

namespace core {

class CommandHandlerTest: public AbstractTest {
};

TEST_F(CommandHandlerTest, testReplacePlaceholders) {
	core::Var::get("somename", "somevalue");
	char buf[512];
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
