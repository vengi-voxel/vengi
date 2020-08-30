/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "util/Console.h"
#include "core/Var.h"
#include "core/command/Command.h"

namespace util {

class ConsoleTest: public core::AbstractTest {
};

class TestConsole : public util::Console {
protected:
	int lineHeight() override { return 0; };
	glm::ivec2 stringSize(const char *c, int length) override { return glm::ivec2(0); }
	void drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) override {}
};

TEST_F(ConsoleTest, testAutoCompleteCvar) {
	const core::String cvar1 = "abcdef_console";
	const core::String cvar2 = "test";
	const core::String cvarComplete = cvar1 + cvar2;
	core::Var::get(cvarComplete, "1");
	TestConsole c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.toggle();
	ASSERT_TRUE(c.isActive());
	ASSERT_TRUE(c.onTextInput(cvar1));
	ASSERT_EQ(cvar1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cvarComplete + " ", c.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteCommand) {
	const core::String cmd1 = "abcdef_console";
	const core::String cmd2 = "test";
	const core::String cmdComplete = cmd1 + cmd2;
	core::Command::registerCommand(cmdComplete.c_str(), [] (const core::CmdArgs& args) {});
	TestConsole c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.toggle();
	ASSERT_TRUE(c.isActive());
	ASSERT_TRUE(c.onTextInput(cmd1));
	ASSERT_EQ(cmd1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cmdComplete + " ", c.commandLine());
}

}
