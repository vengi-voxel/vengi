/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/command/Command.h"

namespace core {

class CommandTest: public AbstractTest {
};

TEST_F(CommandTest, testExecuteUnknown) {
	EXPECT_EQ(0, Command::execute("test"));
}

TEST_F(CommandTest, testExecuteAfterUnregister) {
	Command::registerCommand("test", [&] (const core::CmdArgs&) {
	});
	EXPECT_EQ(1, Command::execute("test"));
	EXPECT_TRUE(Command::unregisterCommand("test")) << "Failed to unregister the 'test' command";
	EXPECT_EQ(0, Command::execute("test"));
}

TEST_F(CommandTest, testExecuteRegistered) {
	int reallyExecuted = 0;
	Command::registerCommand("test", [&] (const core::CmdArgs&) {
		++reallyExecuted;
	});
	EXPECT_EQ(1, Command::execute("test"));
	EXPECT_EQ(1, reallyExecuted);
}

TEST_F(CommandTest, textExecuteParameter) {
	core::String parameter = "command not executed at all";
	Command::registerCommand("test", [&] (const core::CmdArgs&) {
	});
	Command::registerCommand("testparameter", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			parameter = "empty";
		} else {
			parameter = args[0];
		}
	});
	EXPECT_EQ(1, Command::execute("testparameter 42"));
	EXPECT_EQ("42", parameter);
	parameter = "command not executed at all";
	EXPECT_EQ(3, Command::execute("test;testparameter 42; test"));
	EXPECT_EQ("42", parameter);
}

TEST_F(CommandTest, textExecuteSemicolonAsParameter) {
	core::String parameter = "command not executed at all";
	int testExecuted = 0;
	Command::registerCommand("test", [&] (const core::CmdArgs&) {
		++testExecuted;
	});
	Command::registerCommand("testsemicolon", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			parameter = "empty";
		} else {
			parameter = args[0];
		}
	});
	EXPECT_EQ(1, Command::execute(";;;;testsemicolon \";\";;;;"));
	EXPECT_EQ(";", parameter);
	EXPECT_EQ(3, Command::execute("test;;;;testsemicolon \";\";;;;test"));
	EXPECT_EQ(2, testExecuted);
	testExecuted = 0;
	EXPECT_EQ(";", parameter);
	EXPECT_EQ(3, Command::execute("testsemicolon \";\";test parameter; test"));
	EXPECT_EQ(2, testExecuted);
	EXPECT_EQ(";", parameter);
}

}
