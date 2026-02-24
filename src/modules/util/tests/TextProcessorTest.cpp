/**
 * @file
 */

#include "util/TextProcessor.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "core/Var.h"
#include "util/KeybindingHandler.h"

class TextProcessorTest : public app::AbstractTest {
public:
	virtual void SetUp() override {
		core::Var::registerVar(core::VarDef("testReplaceCvar", "value"));
		command::Command::registerCommand("testReplaceCmd")
			.setHandler([](const command::CommandArgs &) {
			}).setHelp("help for cmd");
	}

	virtual void TearDown() override {
		core::Var::shutdown();
		command::Command::shutdown();
	}
};

TEST_F(TextProcessorTest, testReplaceCvar) {
	const core::String str = "value of testReplaceCvar: <cvar:testReplaceCvar>";
	char buf[4096] = "";
	util::KeyBindingHandler handler;
	ASSERT_TRUE(util::replacePlaceholders(handler, str, buf, sizeof(buf)));
	EXPECT_STREQ("value of testReplaceCvar: value", buf);
}

TEST_F(TextProcessorTest, testReplaceCommand) {
	const core::String str = "binding of testReplaceCmd: <cmd:testReplaceCmd>";
	char buf[4096] = "";
	util::KeyBindingHandler handler;
	handler.registerBinding("tab", "testReplaceCmd", "all");
	ASSERT_TRUE(util::replacePlaceholders(handler, str, buf, sizeof(buf)));
	EXPECT_STREQ("binding of testReplaceCmd: tab", buf);
}

TEST_F(TextProcessorTest, testReplace) {
	const core::String str =
		"binding of testReplaceCmd: <cmd:testReplaceCmd> and value of testReplaceCvar: <cvar:testReplaceCvar>";
	char buf[4096] = "";
	util::KeyBindingHandler handler;
	handler.registerBinding("tab", "testReplaceCmd", "all");
	ASSERT_TRUE(util::replacePlaceholders(handler, str, buf, sizeof(buf)));
	EXPECT_STREQ("binding of testReplaceCmd: tab and value of testReplaceCvar: value", buf);
}
