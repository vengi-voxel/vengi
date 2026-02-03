/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/BindingContext.h"
#include "util/CustomButtonNames.h"
#include "util/KeybindingHandler.h"
#include "command/Command.h"
#include "core/Log.h"
#include <SDL_keycode.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDLK_w SDLK_W
#define SDLK_a SDLK_A
#define SDLK_b SDLK_B
#define KMOD_NUM SDL_KMOD_NUM
#endif

namespace util {

namespace keybindingtest {
static const core::String CFG = R"(
w +foo foo
left_alt+w "somecommand +" foo
RIGHT_CTRL+a +bar foo
RIGHT_CTRL+a +foo foo
CTRL+w +bar foo
SHIFT+w +xyz foo
SHIFT+ctrl+ALT+w allmodscommand foo
ctrl+SHIFT+w ctrlshiftmodcommand foo
left_alt altmodcommand foo
double_left_mouse doubleleftclick foo
)";
}

class KeybindingHandlerTest : public app::AbstractTest {
protected:
	KeyBindingHandler _handler;
	bool _allmodscommand = false;
	bool _ctrlshiftmodcommand = false;
	bool _somecommand = false;
	bool _altmodcommand = false;
	bool _foo = false;
	bool _xyz = false;
	bool _doubleLeftClick = false;

	void reset() {
		_xyz = _ctrlshiftmodcommand = _somecommand = _altmodcommand = _allmodscommand = _foo = _doubleLeftClick = false;
	}

	bool onInitApp() override {
		core::registerBindingContext("all", core::BindingContext::All);
		core::registerBindingContext("foo", core::BindingContext::Context1);
		core::registerBindingContext("bar", core::BindingContext::Context2);
		KeybindingParser _parser(keybindingtest::CFG);
		if (!app::AbstractTest::onInitApp()) {
			return false;
		}
		if (_parser.invalidBindings() > 0) {
			Log::error("Not all bindings could get parsed. Invalid bindings: %i", _parser.invalidBindings());
			return false;
		}
		_handler.construct();
		if (!_handler.init()) {
			Log::error("Failed to initialize the key binding handler");
			return false;
		}
		_handler.setBindings(_parser.getBindings());
		reset();
		core::setBindingContext(core::BindingContext::Context1);
		command::Command::shutdown();
		command::Command::registerCommand("+bar").setHandler([] (const command::CommandArgs& args) {});
		command::Command::registerCommand("-bar").setHandler([] (const command::CommandArgs& args) {});
		command::Command::registerCommand("+foo").setHandler([this] (const command::CommandArgs& args) {this->_foo = true;});
		command::Command::registerCommand("-foo").setHandler([this] (const command::CommandArgs& args) {this->_foo = false;});
		command::Command::registerCommand("+xyz").setHandler([this] (const command::CommandArgs& args) {this->_xyz = true;});
		command::Command::registerCommand("-xyz").setHandler([this] (const command::CommandArgs& args) {this->_xyz = false;});
		command::Command::registerCommand("somecommand").setHandler([this] (const command::CommandArgs& args) {this->_somecommand = true;});
		command::Command::registerCommand("altmodcommand").setHandler([this] (const command::CommandArgs& args) {this->_altmodcommand = true;});
		command::Command::registerCommand("allmodscommand").setHandler([this] (const command::CommandArgs& args) {this->_allmodscommand = true;});
		command::Command::registerCommand("ctrlshiftmodcommand").setHandler([this] (const command::CommandArgs& args) {this->_ctrlshiftmodcommand = true;});
		command::Command::registerCommand("doubleleftclick").setHandler([this] (const command::CommandArgs& args) {this->_doubleLeftClick = true;});
		return true;
	}

	void onCleanupApp() override {
		app::AbstractTest::onCleanupApp();
		_handler.shutdown(0);
		core::resetBindingContexts();
	}

	void execute(int32_t key, int16_t modifier = 0, bool pressed = true, uint16_t count = 1u) {
		reset();
		EXPECT_TRUE(_handler.execute(key, modifier, pressed, 0.0, count))
				<< "Command for key '" << KeyBindingHandler::toString(key, modifier, count) << "' should be executed";
	}

	void notExecute(int32_t key, int16_t modifier = 0, bool pressed = true, uint16_t count = 1u) {
		reset();
		EXPECT_FALSE(_handler.execute(key, modifier, pressed, 0.0, count))
				<< "Command for key '" << KeyBindingHandler::toString(key, modifier, count) << "' should not be executed";
	}

	/**
	 * for +commandname bindings
	 */
	void executeActionButtonCommand(int32_t key, int16_t modifier = KMOD_NONE, bool pressed = true) {
		reset();
		execute(key, modifier, pressed);
	}
};

TEST_F(KeybindingHandlerTest, testValidCommandNoModifiers) {
	executeActionButtonCommand(SDLK_w, KMOD_NONE, true);
	EXPECT_TRUE(_foo) << "expected command wasn't executed";
	EXPECT_TRUE(_handler.isPressed(SDLK_w));
	executeActionButtonCommand(SDLK_w, KMOD_NONE, false);
	EXPECT_FALSE(_handler.isPressed(SDLK_w));
}

TEST_F(KeybindingHandlerTest, testNotBoundKey) {
	notExecute(SDLK_b);
}

TEST_F(KeybindingHandlerTest, testLeftAltModifier) {
	execute(SDLK_w, KMOD_LALT);
	EXPECT_TRUE(_somecommand) << "expected command wasn't executed";
	EXPECT_FALSE(_allmodscommand) << "unexpected command was executed";
}

TEST_F(KeybindingHandlerTest, testRightAltModifier) {
	execute(SDLK_w, KMOD_RALT);
	EXPECT_TRUE(_foo) << "expected command wasn't executed - there is no right_alt+w bound, just w";
	EXPECT_FALSE(_somecommand) << "unexpected command was executed";
}

TEST_F(KeybindingHandlerTest, testAltKey) {
	execute(SDLK_LALT, KMOD_NONE);
	EXPECT_TRUE(_altmodcommand) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testLeftShiftModifier) {
	execute(SDLK_w, KMOD_LSHIFT);
	EXPECT_TRUE(_xyz) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testAllValidModifier) {
	execute(SDLK_w, KMOD_LSHIFT | KMOD_LCONTROL | KMOD_LALT);
	EXPECT_TRUE(_allmodscommand) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testAllValidModifier2) {
	execute(SDLK_w, KMOD_RSHIFT | KMOD_LCONTROL | KMOD_RALT);
	EXPECT_TRUE(_allmodscommand) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testCtrlShiftModifier) {
	execute(SDLK_w, KMOD_LSHIFT | KMOD_LCONTROL);
	EXPECT_TRUE(_ctrlshiftmodcommand) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testLShiftRCtrlModifier) {
	execute(SDLK_w, KMOD_LSHIFT | KMOD_RCONTROL);
	EXPECT_TRUE(_ctrlshiftmodcommand) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testRightShiftModifier) {
	execute(SDLK_w, KMOD_RSHIFT);
	EXPECT_TRUE(_xyz) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testRightShiftModifierWrongContext) {
	core::setBindingContext(core::BindingContext::Context2);
	notExecute(SDLK_w, KMOD_RSHIFT);
	core::setBindingContext(core::BindingContext::Context1);
}

TEST_F(KeybindingHandlerTest, testShiftModifier) {
	execute(SDLK_w, KMOD_LSHIFT);
	EXPECT_TRUE(_xyz) << "expected command wasn't executed";
}

TEST_F(KeybindingHandlerTest, testDoubleLeftClick) {
	notExecute(button::CUSTOM_SDLK_MOUSE_LEFT, 0, true, 1);
	EXPECT_FALSE(_doubleLeftClick) << "the command should not get executed on single click events";
	execute(button::CUSTOM_SDLK_MOUSE_LEFT, 0, true, 2);
	EXPECT_TRUE(_doubleLeftClick) << "the command should get executed on double click events";
	notExecute(button::CUSTOM_SDLK_MOUSE_LEFT, 0, true, 1);
	EXPECT_FALSE(_doubleLeftClick) << "the command should not get executed on single click events";
}

TEST_F(KeybindingHandlerTest, testCtrlModifierA) {
	executeActionButtonCommand(SDLK_a, KMOD_RCONTROL);
}

TEST_F(KeybindingHandlerTest, testCtrlModifierAWrongModifierPressed) {
	notExecute(SDLK_a, KMOD_LCONTROL);
}

TEST_F(KeybindingHandlerTest, testCtrlModifier) {
	executeActionButtonCommand(SDLK_w, KMOD_LCONTROL);
}

TEST_F(KeybindingHandlerTest, testModifierMasksAltSimple) {
	EXPECT_TRUE(isValidForBinding(KMOD_LALT, KMOD_ALT)) << "lalt pressed - alt bound";
	EXPECT_TRUE(isValidForBinding(KMOD_RALT, KMOD_ALT)) << "ralt pressed - alt bound";

	EXPECT_TRUE(isValidForBinding(KMOD_LALT, KMOD_LALT)) << "lalt pressed - lalt bound";
	EXPECT_FALSE(isValidForBinding(KMOD_RALT, KMOD_LALT)) << "ralt pressed - lalt bound";
}

TEST_F(KeybindingHandlerTest, testModifierMasksShiftSimple) {
	EXPECT_TRUE(isValidForBinding(KMOD_LSHIFT, KMOD_SHIFT)) << "lshift pressed - shift bound";
	EXPECT_TRUE(isValidForBinding(KMOD_RSHIFT, KMOD_SHIFT)) << "rshift pressed - shift bound";

	EXPECT_TRUE(isValidForBinding(KMOD_LSHIFT, KMOD_LSHIFT)) << "lshift pressed - lshift bound";
	EXPECT_FALSE(isValidForBinding(KMOD_RSHIFT, KMOD_LSHIFT)) << "rshift pressed - lshift bound";
}

TEST_F(KeybindingHandlerTest, testModifierMasksCtrlSimple) {
	EXPECT_TRUE(isValidForBinding(KMOD_LCONTROL, KMOD_CONTROL)) << "lctrl pressed - ctrl bound";
	EXPECT_TRUE(isValidForBinding(KMOD_RCONTROL, KMOD_CONTROL)) << "rctrl pressed - ctrl bound";

	EXPECT_TRUE(isValidForBinding(KMOD_LCONTROL, KMOD_LCONTROL)) << "lctrl pressed - lctrl bound";
	EXPECT_FALSE(isValidForBinding(KMOD_RCONTROL, KMOD_LCONTROL)) << "rctrl pressed - lctrl bound";
}

TEST_F(KeybindingHandlerTest, testModifierMasksInvalidModifiers) {
	EXPECT_TRUE(isValidForBinding(KMOD_LALT | KMOD_NUM, KMOD_ALT)) << "lalt pressed - alt bound";
	EXPECT_TRUE(isValidForBinding(KMOD_RALT | KMOD_NUM, KMOD_ALT)) << "ralt pressed - alt bound";

	EXPECT_TRUE(isValidForBinding(KMOD_LALT | KMOD_NUM, KMOD_LALT)) << "lalt pressed - lalt bound";
	EXPECT_FALSE(isValidForBinding(KMOD_RALT | KMOD_NUM, KMOD_LALT)) << "ralt pressed - lalt bound";
}

}
