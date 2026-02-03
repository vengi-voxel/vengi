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
	command::Command::registerCommand("testquitewhitespace")
		.addArg({"param", ArgType::String, true, ""})
		.setHandler([&](const CommandArgs &args) {
			const core::String &param = args.str("param");
			if (!param.empty()) {
				parameter = param;
			}
		});

	EXPECT_EQ(-1, command::executeCommands("testquitewhitespace \"foobar barfoo\";nocommand"));
	EXPECT_STREQ("foobar barfoo", parameter.c_str());
}

} // namespace command
