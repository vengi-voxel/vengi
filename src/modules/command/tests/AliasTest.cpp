/**
 * @file
 */

#include "command/Alias.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include <gtest/gtest.h>

namespace command {

class AliasTest : public testing::Test {
public:
	void SetUp() override {
		Alias::construct();
	}

	void TearDown() override {
		Command::shutdown();
		core::Var::shutdown();
	}
};

TEST_F(AliasTest, testSetAndExecute) {
	int executed = 0;
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
			++executed;
		});

	ASSERT_TRUE(Alias::set("foo", "testcmd"));
	EXPECT_TRUE(Alias::has("foo"));
	EXPECT_EQ("testcmd", Alias::get("foo"));
	EXPECT_NE(nullptr, Command::getCommand("foo"));
	EXPECT_EQ(1, Command::execute("foo"));
	EXPECT_EQ(1, executed);
}

TEST_F(AliasTest, testMultipleCommands) {
	int a = 0;
	int b = 0;
	Command::registerCommand("cmda")
		.setHandler([&](const CommandArgs &) {
			++a;
		});
	Command::registerCommand("cmdb")
		.setHandler([&](const CommandArgs &) {
			++b;
		});

	ASSERT_TRUE(Alias::set("combo", "cmda; cmdb"));
	EXPECT_EQ(1, Command::execute("combo"));
	EXPECT_EQ(1, a);
	EXPECT_EQ(1, b);
}

TEST_F(AliasTest, testAliasCommand) {
	int executed = 0;
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
			++executed;
		});

	EXPECT_EQ(1, Command::execute("alias foo testcmd"));
	EXPECT_TRUE(Alias::has("foo"));
	EXPECT_EQ(1, Command::execute("foo"));
	EXPECT_EQ(1, executed);
}

TEST_F(AliasTest, testAliasCommandQuoted) {
	int a = 0;
	int b = 0;
	Command::registerCommand("cmda")
		.setHandler([&](const CommandArgs &) {
			++a;
		});
	Command::registerCommand("cmdb")
		.setHandler([&](const CommandArgs &) {
			++b;
		});

	EXPECT_EQ(1, Command::execute("alias combo \"cmda; cmdb\""));
	EXPECT_EQ("cmda; cmdb", Alias::get("combo"));
	EXPECT_EQ(1, Command::execute("combo"));
	EXPECT_EQ(1, a);
	EXPECT_EQ(1, b);
}

TEST_F(AliasTest, testUnaliasCommand) {
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
		});
	ASSERT_TRUE(Alias::set("foo", "testcmd"));
	EXPECT_EQ(0, Command::execute("unalias"));
	EXPECT_TRUE(Alias::has("foo"));
	EXPECT_EQ(1, Command::execute("unalias foo"));
	EXPECT_FALSE(Alias::has("foo"));
	EXPECT_EQ(nullptr, Command::getCommand("foo"));
	EXPECT_EQ(0, Command::execute("foo"));
}

TEST_F(AliasTest, testCannotOverwriteCommand) {
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
		});
	EXPECT_FALSE(Alias::set("testcmd", "alias"));
	EXPECT_FALSE(Alias::has("testcmd"));
}

TEST_F(AliasTest, testCannotOverwriteCvar) {
	core::Var::registerVar(core::VarDef("testvar", "1", nullptr, nullptr));
	EXPECT_FALSE(Alias::set("testvar", "alias"));
	EXPECT_FALSE(Alias::has("testvar"));
}

TEST_F(AliasTest, testUpdateExistingAlias) {
	int first = 0;
	int second = 0;
	Command::registerCommand("firstcmd")
		.setHandler([&](const CommandArgs &) {
			++first;
		});
	Command::registerCommand("secondcmd")
		.setHandler([&](const CommandArgs &) {
			++second;
		});

	ASSERT_TRUE(Alias::set("foo", "firstcmd"));
	EXPECT_EQ(1, Command::execute("foo"));
	EXPECT_EQ(1, first);
	EXPECT_EQ(0, second);

	ASSERT_TRUE(Alias::set("foo", "secondcmd"));
	EXPECT_EQ(1, Command::execute("foo"));
	EXPECT_EQ(1, first);
	EXPECT_EQ(1, second);
}

TEST_F(AliasTest, testInvalidName) {
	EXPECT_FALSE(Alias::set("", "testcmd"));
	EXPECT_FALSE(Alias::set("alias", "testcmd"));
	EXPECT_FALSE(Alias::set("unalias", "testcmd"));
	EXPECT_FALSE(Alias::set("+foo", "testcmd"));
	EXPECT_FALSE(Alias::set("foo bar", "testcmd"));
	EXPECT_FALSE(Alias::set("foo;bar", "testcmd"));
}

TEST_F(AliasTest, testEmptyCommands) {
	EXPECT_FALSE(Alias::set("foo", ""));
}

TEST_F(AliasTest, testSerializeParseRoundtrip) {
	int executed = 0;
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
			++executed;
		});
	Command::registerCommand("other")
		.setHandler([&](const CommandArgs &) {
		});

	ASSERT_TRUE(Alias::set("foo", "testcmd"));
	ASSERT_TRUE(Alias::set("bar", "testcmd; other"));
	const core::String serialized = Alias::serialize();
	EXPECT_TRUE(serialized.contains("foo"));
	EXPECT_TRUE(serialized.contains("bar"));

	ASSERT_TRUE(Alias::remove("foo"));
	ASSERT_TRUE(Alias::remove("bar"));
	EXPECT_EQ(0u, Alias::size());

	ASSERT_TRUE(Alias::parse(serialized));
	EXPECT_EQ(2u, Alias::size());
	EXPECT_EQ("testcmd", Alias::get("foo"));
	EXPECT_EQ("testcmd; other", Alias::get("bar"));
	EXPECT_EQ(1, Command::execute("foo"));
	EXPECT_EQ(1, executed);
}

TEST_F(AliasTest, testSerializeQuotedCommands) {
	Command::registerCommand("echo")
		.addArg({"message", ArgType::String, true, ""})
		.setHandler([&](const CommandArgs &) {
		});
	ASSERT_TRUE(Alias::set("greet", "echo \"hello world\""));
	const core::String serialized = Alias::serialize();
	ASSERT_TRUE(Alias::remove("greet"));
	ASSERT_TRUE(Alias::parse(serialized));
	EXPECT_EQ("echo \"hello world\"", Alias::get("greet"));
}

TEST_F(AliasTest, testRecursionLimit) {
	ASSERT_TRUE(Alias::set("a", "b"));
	ASSERT_TRUE(Alias::set("b", "a"));
	Command::execute("a");
}

TEST_F(AliasTest, testExecuteCommandsHandler) {
	int executed = 0;
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
			++executed;
		});
	ASSERT_TRUE(Alias::set("foo", "testcmd"));
	EXPECT_EQ(1, executeCommands("foo"));
	EXPECT_EQ(1, executed);
}

TEST_F(AliasTest, testExecuteCommandsCreateAlias) {
	int a = 0;
	int b = 0;
	Command::registerCommand("cmda")
		.setHandler([&](const CommandArgs &) {
			++a;
		});
	Command::registerCommand("cmdb")
		.setHandler([&](const CommandArgs &) {
			++b;
		});

	EXPECT_EQ(1, executeCommands("alias combo \"cmda; cmdb\""));
	EXPECT_EQ("cmda; cmdb", Alias::get("combo"));
	EXPECT_EQ(1, executeCommands("combo"));
	EXPECT_EQ(1, a);
	EXPECT_EQ(1, b);
}

TEST_F(AliasTest, testBindableAsCommand) {
	int executed = 0;
	Command::registerCommand("testcmd")
		.setHandler([&](const CommandArgs &) {
			++executed;
		});
	ASSERT_TRUE(Alias::set("mynode", "testcmd"));
	const Command *cmd = Command::getCommand("mynode");
	ASSERT_NE(nullptr, cmd);
	EXPECT_TRUE(cmd->help().contains("testcmd"));
	EXPECT_EQ(1, Command::execute("mynode"));
	EXPECT_EQ(1, executed);
}

} // namespace command
