/**
 * @file
 */

#include "util/Console.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "core/Var.h"
#include <SDL_version.h>
#include <SDL_log.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_LogGetOutputFunction SDL_GetLogOutputFunction
#define SDL_LogSetOutputFunction SDL_SetLogOutputFunction
#endif

namespace util {

class ConsoleTest : public app::AbstractTest {
protected:
	class TestConsole : public util::Console {
	public:
		TestConsole(const core::String &commandLine) {
			_commandLine = commandLine;
		}
	};
};

TEST_F(ConsoleTest, testAutoCompleteCvar) {
	const core::String cvar1 = "abcdef_console";
	const core::String cvar2 = "test";
	const core::String cvarComplete = cvar1 + cvar2;
	core::Var::get(cvarComplete, "1");
	TestConsole c(cvar1);
	SDL_LogSetOutputFunction(nullptr, nullptr);
	ASSERT_EQ(cvar1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cvarComplete + " ", c.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteCommand) {
	const core::String cmd1 = "abcdef_console";
	const core::String cmd2 = "test";
	const core::String cmdComplete = cmd1 + cmd2;
	command::Command::registerCommand(cmdComplete.c_str())
		.setHandler([](const command::CommandArgs &args) {});
	TestConsole c(cmd1);
	SDL_LogSetOutputFunction(nullptr, nullptr);
	ASSERT_EQ(cmd1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cmdComplete + " ", c.commandLine());
}

} // namespace util
