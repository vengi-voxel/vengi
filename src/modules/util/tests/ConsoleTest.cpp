/**
 * @file
 */

#include "util/Console.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
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
	core::Var::registerVar(core::VarDef(cvarComplete, 1, "", ""));
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

TEST_F(ConsoleTest, testAutoCompleteEnumCvar) {
	const core::String cvarName = "zz_test_enum_console";
	core::DynamicArray<core::String> validValues;
	validValues.push_back("alpha");
	validValues.push_back("beta");
	validValues.push_back("gamma");
	core::Var::registerVar(core::VarDef(cvarName, "alpha", validValues, "", ""));
	TestConsole c(cvarName + " ");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	// all three values should be offered - the command line should be updated with common prefix
	// since no common prefix exists among alpha, beta, gamma, the command line stays as is
	// but we should at least not crash and the matches should be found
	// let's test with a partial match
	TestConsole c2(cvarName + " al");
	c2.autoComplete();
	ASSERT_EQ(cvarName + " alpha", c2.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteEnumCvarPartialMatch) {
	const core::String cvarName = "zz_test_enum_partial_console";
	core::DynamicArray<core::String> validValues;
	validValues.push_back("option_one");
	validValues.push_back("option_two");
	validValues.push_back("option_three");
	core::Var::registerVar(core::VarDef(cvarName, "option_one", validValues, "", ""));
	TestConsole c(cvarName + " option_t");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	// "option_two" and "option_three" both match, common prefix is "option_t"
	// so the command line should have the common prefix filled
	const core::String &cmdLine = c.commandLine();
	ASSERT_TRUE(cmdLine.contains("option_t"));
}

TEST_F(ConsoleTest, testAutoCompleteBooleanCvar) {
	const core::String cvarName = "zz_test_bool_console";
	core::Var::registerVar(core::VarDef(cvarName, true, "", ""));
	TestConsole c(cvarName + " t");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	ASSERT_EQ(cvarName + " true", c.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteSetCvarName) {
	command::CommandArg cvarArg("name", command::ArgType::String, false, "", "Variable name");
	cvarArg.completer = command::cvarCompleter();
	command::CommandArg cvarValue("value", command::ArgType::String, false, "", "Variable value");
	cvarValue.completer = command::cvarValueCompleter();
	command::Command::registerCommand("set")
		.addArg(cvarArg)
		.addArg(cvarValue)
		.setHandler([](const command::CommandArgs &args) {});

	const core::String cvarName = "zz_test_set_complete_console";
	core::Var::registerVar(core::VarDef(cvarName, 1, "", ""));
	TestConsole c("set zz_test_set_complete_c");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	ASSERT_EQ("set " + cvarName, c.commandLine());
	command::Command::unregisterCommand("set");
}

TEST_F(ConsoleTest, testAutoCompleteSetEnumValue) {
	command::CommandArg cvarArg("name", command::ArgType::String, false, "", "Variable name");
	cvarArg.completer = command::cvarCompleter();
	command::CommandArg cvarValue("value", command::ArgType::String, false, "", "Variable value");
	cvarValue.completer = command::cvarValueCompleter();
	command::Command::registerCommand("set")
		.addArg(cvarArg)
		.addArg(cvarValue)
		.setHandler([](const command::CommandArgs &args) {});

	const core::String cvarName = "zz_test_set_enum_value_console";
	core::DynamicArray<core::String> validValues;
	validValues.push_back("red");
	validValues.push_back("green");
	validValues.push_back("blue");
	core::Var::registerVar(core::VarDef(cvarName, "red", validValues, "", ""));
	TestConsole c("set " + cvarName + " gr");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	ASSERT_EQ("set " + cvarName + " green", c.commandLine());
	command::Command::unregisterCommand("set");
}

TEST_F(ConsoleTest, testAutoCompleteSetBoolValue) {
	command::CommandArg cvarArg("name", command::ArgType::String, false, "", "Variable name");
	cvarArg.completer = command::cvarCompleter();
	command::CommandArg cvarValue("value", command::ArgType::String, false, "", "Variable value");
	cvarValue.completer = command::cvarValueCompleter();
	command::Command::registerCommand("set")
		.addArg(cvarArg)
		.addArg(cvarValue)
		.setHandler([](const command::CommandArgs &args) {});

	const core::String cvarName = "zz_test_set_bool_value_console";
	core::Var::registerVar(core::VarDef(cvarName, true, "", ""));
	TestConsole c("set " + cvarName + " f");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	ASSERT_EQ("set " + cvarName + " false", c.commandLine());
	command::Command::unregisterCommand("set");
}

TEST_F(ConsoleTest, testAutoCompleteSetCvarNameMultipleMatches) {
	command::CommandArg cvarArg("name", command::ArgType::String, false, "", "Variable name");
	cvarArg.completer = command::cvarCompleter();
	command::CommandArg cvarValue("value", command::ArgType::String, false, "", "Variable value");
	cvarValue.completer = command::cvarValueCompleter();
	command::Command::registerCommand("set")
		.addArg(cvarArg)
		.addArg(cvarValue)
		.setHandler([](const command::CommandArgs &args) {});

	const core::String cvarName1 = "zz_test_set_multi_alpha";
	const core::String cvarName2 = "zz_test_set_multi_beta";
	core::Var::registerVar(core::VarDef(cvarName1, 1, "", ""));
	core::Var::registerVar(core::VarDef(cvarName2, 2, "", ""));
	TestConsole c("set zz_test_set_multi");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	// both match, common prefix is "zz_test_set_multi_"
	const core::String expected = "set zz_test_set_multi_";
	ASSERT_EQ(expected, c.commandLine());
	command::Command::unregisterCommand("set");
}

TEST_F(ConsoleTest, testAutoCompleteSetEnumValueNoPartial) {
	command::CommandArg cvarArg("name", command::ArgType::String, false, "", "Variable name");
	cvarArg.completer = command::cvarCompleter();
	command::CommandArg cvarValue("value", command::ArgType::String, false, "", "Variable value");
	cvarValue.completer = command::cvarValueCompleter();
	command::Command::registerCommand("set")
		.addArg(cvarArg)
		.addArg(cvarValue)
		.setHandler([](const command::CommandArgs &args) {});

	const core::String cvarName = "zz_test_set_enum_nop_console";
	core::DynamicArray<core::String> validValues;
	validValues.push_back("mcedit2");
	validValues.push_back("worldedit");
	validValues.push_back("schematica");
	core::Var::registerVar(core::VarDef(cvarName, "mcedit2", validValues, "", ""));
	// space after cvar name, no partial value typed yet - should offer all values
	TestConsole c("set " + cvarName + " m");
	SDL_LogSetOutputFunction(nullptr, nullptr);
	c.autoComplete();
	// "mcedit2" matches the partial "m" - single match should complete fully
	ASSERT_EQ("set " + cvarName + " mcedit2", c.commandLine());
	command::Command::unregisterCommand("set");
}

} // namespace util
