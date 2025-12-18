/**
 * @file
 */

#include "util/KeybindingParser.h"
#include "app/tests/AbstractTest.h"
#include "util/CustomButtonNames.h"
#include "util/KeybindingHandler.h"
#include <SDL_keycode.h>

namespace util {

inline std::ostream &operator<<(::std::ostream &os, const BindMap &dt) {
	os << "\n";
	for (auto i = dt.begin(); i != dt.end(); ++i) {
		const core::String &name = KeyBindingHandler::toString(i->first, i->second.modifier, i->second.count);
		os << "\t" << name << " -> " << i->second.command << " (context: " << i->second.context << ")\n";
	}
	return os;
}

namespace keybindingparser {
static const core::String CFG = R"(
w +foo all
alt+w "somecommand +" all
left_alt+l "someothercommand +" all
CTRL+a +bar all
CTRL+w +bar all
SHIFT+w +xyz all
SHIFT+ctrl+ALT+w allmodscommand all
ctrl+SHIFT+w ctrlshiftmodcommand all
left_alt altmodcommand all
ctrl++ "echo +" all
left_mouse void all
right_mouse void all
double_right_mouse void all
wheelup void all
wheeldown void all
left_gui void all
+ "echo only+" all
)";
}

class KeybindingParserTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	bool onInitApp() override {
		if (!Super::onInitApp()) {
			return false;
		}
		core::registerBindingContext("all", core::BindingContext::All);
		return true;
	}
};

TEST_F(KeybindingParserTest, testParsing) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap &m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings()) << p.lastError();
	const size_t expected = 17;
	EXPECT_EQ(expected, m.size());

	int key = 'w';
	int count = 0;
	auto range = m.equal_range(key);
	bool allmodscommand = false;
	bool ctrlshiftmodcommand = false;
	bool xyz = false;
	bool bar = false;
	bool somecommand = false;
	bool foo = false;
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair &pair = i->second;
		const core::String &command = pair.command;
		const int16_t mod = pair.modifier;
		if ((mod & KMOD_SHIFT) && (mod & KMOD_ALT) && (mod & KMOD_CONTROL)) {
			EXPECT_EQ("allmodscommand", command) << "Expected command 'allmodscommand' but got " << command;
			EXPECT_FALSE(allmodscommand) << "allmodscommand found twice! bindings: " << m;
			allmodscommand = true;
		} else if ((mod & KMOD_SHIFT) && (mod & KMOD_CONTROL)) {
			EXPECT_EQ("ctrlshiftmodcommand", command) << "Expected command 'ctrlshiftmodcommand' but got " << command;
			EXPECT_FALSE(ctrlshiftmodcommand) << "ctrlshiftmodcommand found twice! bindings: " << m;
			ctrlshiftmodcommand = true;
		} else if (mod & KMOD_SHIFT) {
			EXPECT_EQ("+xyz", command) << "Expected command '+xyz' but got " << command;
			EXPECT_FALSE(xyz) << "+xyz found twice! bindings: " << m;
			xyz = true;
		} else if (mod & KMOD_CONTROL) {
			EXPECT_EQ("+bar", command) << "Expected command '+bar' but got " << command;
			EXPECT_FALSE(bar) << "+bar found twice! bindings: " << m;
			bar = true;
		} else if (mod & KMOD_ALT) {
			EXPECT_EQ("somecommand +", command) << "Expected command 'somecommand +' but got " << command;
			EXPECT_FALSE(somecommand) << "somecommand + found twice! bindings: " << m;
			somecommand = true;
		} else {
			EXPECT_EQ("+foo", command) << "Expected command '+foo' but got " << command;
			EXPECT_FALSE(foo) << "+foo found twice! bindings: " << m;
			foo = true;
		}
		++count;
	}

	ASSERT_TRUE(allmodscommand) << "allmodscommand not found! bindings: " << m;
	ASSERT_TRUE(ctrlshiftmodcommand) << "ctrlshiftmodcommand not found! bindings: " << m;
	ASSERT_TRUE(xyz) << "xyz not found! bindings: " << m;
	ASSERT_TRUE(bar) << "bar not found! bindings: " << m;
	ASSERT_TRUE(somecommand) << "somecommand not found! bindings: " << m;
	ASSERT_TRUE(foo) << "foo not found! bindings: " << m;
	EXPECT_EQ(6, count) << "expected 6 bindings for key " << key << " but got " << count;
}

TEST_F(KeybindingParserTest, testParsing2) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap &m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings()) << p.lastError();
	int key = 'l';
	int count = 0;
	auto range = m.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair &pair = i->second;
		const core::String &command = pair.command;
		const int16_t mod = pair.modifier;
		EXPECT_EQ("someothercommand +", command)
			<< "Expected command 'someothercommand +' but got " << command << "! bindings: " << m;
		EXPECT_TRUE(mod & KMOD_LALT) << "command " << command << " modifier wasn't parsed properly! bindings: " << m;
		EXPECT_TRUE(!(mod & KMOD_RALT)) << "command " << command << " modifier wasn't parsed properly! bindings: " << m;
		++count;
	}
	EXPECT_EQ(1, count) << "expected 1 binding for key " << key << " but got " << count << "! bindings: " << m;
}

TEST_F(KeybindingParserTest, testParsing3) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap &m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings()) << p.lastError();

	const int key = '+';
	bool echoPlusFound = false;
	bool echoOnlyPlusFound = false;
	auto range = m.equal_range(key);
	int count = 0;
	ASSERT_TRUE(range.first != range.second) << "no binding found for key " << (char)key << "! bindings: " << m;
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair &pair = i->second;
		const core::String &command = pair.command;
		const int16_t mod = pair.modifier;
		if ((mod & KMOD_CONTROL)) {
			EXPECT_EQ("echo +", command);
			EXPECT_TRUE(mod & KMOD_CONTROL)
				<< "command " << command << " modifier wasn't parsed properly! bindings: " << m;
			echoPlusFound = true;
		} else {
			if (mod == 0) {
				EXPECT_EQ("echo only+", command);
				echoOnlyPlusFound = true;
			}
		}
		++count;
	}
	EXPECT_TRUE(echoOnlyPlusFound) << "echo only+ not found in " << count << " bindings! bindings: " << m;
	EXPECT_TRUE(echoPlusFound) << "echo + not found in " << count << " bindings! bindings: " << m;
}

TEST_F(KeybindingParserTest, testParsing4) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap &m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings()) << p.lastError();

	int key = SDLK_LALT;
	int count = 0;
	auto range = m.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair &pair = i->second;
		const core::String &command = pair.command;
		const int16_t mod = pair.modifier;
		EXPECT_EQ("altmodcommand", command);
		EXPECT_EQ(0, mod);
		++count;
	}
	EXPECT_EQ(1, count) << "expected 1 binding for key " << key << " but got " << count;
}

} // namespace util
