/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "ui/turbobadger/Console.h"
#include "core/Var.h"
#include "core/command/Command.h"

namespace ui {

class ConsoleTest: public core::AbstractTest {
};

TEST_F(ConsoleTest, testAutoCompleteCvar) {
	const std::string cvar1 = "abcdef_console";
	const std::string cvar2 = "test";
	const std::string cvarComplete = cvar1 + cvar2;
	core::Var::get(cvarComplete, "1");
	Console c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.toggle();
	ASSERT_TRUE(c.isActive());
	ASSERT_TRUE(c.onTextInput(cvar1));
	ASSERT_EQ(cvar1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cvarComplete + " ", c.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteCommand) {
	const std::string cmd1 = "abcdef_console";
	const std::string cmd2 = "test";
	const std::string cmdComplete = cmd1 + cmd2;
	core::Command::registerCommand(cmdComplete, [] (const core::CmdArgs& args) {});
	Console c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.toggle();
	ASSERT_TRUE(c.isActive());
	ASSERT_TRUE(c.onTextInput(cmd1));
	ASSERT_EQ(cmd1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cmdComplete + " ", c.commandLine());
}

}
