/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "util/KeybindingParser.h"
#include <SDL.h>

namespace util {

namespace keybindingparser {
static const std::string CFG = R"(
w +foo
alt+w "somecommand +"
left_alt+l "someothercommand +"
CTRL+a +bar
CTRL+w +bar
SHIFT+w +xyz
SHIFT+ctrl+ALT+w allmodscommand
ctrl+SHIFT+w ctrlshiftmodcommand
left_alt altmodcommand
left_mouse void
right_mouse void
wheelup void
wheeldown void
)";
}

TEST(KeybindingParserTest, testParsing) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap& m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings());
	const std::size_t expected = 13;
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
		const CommandModifierPair& pair = i->second;
		const std::string& command = pair.command;
		const int16_t mod = pair.modifier;
		if ((mod & KMOD_SHIFT) && (mod & KMOD_ALT) && (mod & KMOD_CTRL)) {
			EXPECT_EQ("allmodscommand", command);
			EXPECT_FALSE(allmodscommand);
			allmodscommand = true;
		} else if ((mod & KMOD_SHIFT) && (mod & KMOD_CTRL)) {
			EXPECT_EQ("ctrlshiftmodcommand", command);
			EXPECT_FALSE(ctrlshiftmodcommand);
			ctrlshiftmodcommand = true;
		} else if (mod & KMOD_SHIFT) {
			EXPECT_EQ("+xyz", command);
			EXPECT_FALSE(xyz);
			xyz = true;
		} else if (mod & KMOD_CTRL) {
			EXPECT_EQ("+bar", command);
			EXPECT_FALSE(bar);
			bar = true;
		} else if (mod & KMOD_ALT) {
			EXPECT_EQ("somecommand +", command);
			EXPECT_FALSE(somecommand);
			somecommand = true;
		} else {
			EXPECT_EQ("+foo", command);
			EXPECT_FALSE(foo);
			foo = true;
		}
		++count;
	}

	ASSERT_TRUE(allmodscommand);
	ASSERT_TRUE(ctrlshiftmodcommand);
	ASSERT_TRUE(xyz);
	ASSERT_TRUE(bar);
	ASSERT_TRUE(somecommand);
	ASSERT_TRUE(foo);

	const int expectedWBindings = 6;
	EXPECT_EQ(expectedWBindings, count);

	key = 'l';
	count = 0;
	range = m.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair& pair = i->second;
		const std::string& command = pair.command;
		const int16_t mod = pair.modifier;
		EXPECT_EQ("someothercommand +", command);
		EXPECT_TRUE(mod & KMOD_LALT) << "command " << command << " modifier wasn't parsed properly";
		EXPECT_TRUE(!(mod & KMOD_RALT)) << "command " << command << " modifier wasn't parsed properly";
		++count;
	}
	EXPECT_EQ(1, count);

	key = SDLK_LALT;
	count = 0;
	range = m.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair& pair = i->second;
		const std::string& command = pair.command;
		const int16_t mod = pair.modifier;
		EXPECT_EQ("altmodcommand", command);
		EXPECT_EQ(0, mod);
		++count;
	}
	EXPECT_EQ(1, count);

}

}
