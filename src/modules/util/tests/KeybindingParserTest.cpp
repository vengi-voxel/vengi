/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "util/KeybindingParser.h"

namespace util {

namespace keybindingparser {
static const std::string CFG = R"(
w +foo
alt+w "somecommand +"
lalt+l "someothercommand +"
CTRL+a +bar
CTRL+w +bar
SHIFT+w +xyz
SHIFT+ctrl+ALT+w allmodscommand
ctrl+SHIFT+w ctrlshiftmodcommand
)";
}

TEST(KeybindingParserTest, testParsing) {
	KeybindingParser p(keybindingparser::CFG);
	const BindMap& m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings());
	const std::size_t expected = 8;
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
		const std::string& command = i->second.command;
		const int16_t mod = i->second.modifier;
		if ((mod & KMOD_SHIFT) && (mod & KMOD_ALT) && (mod & KMOD_CTRL)) {
			EXPECT_EQ("allmodscommand", command);
			allmodscommand = true;
		} else if ((mod & KMOD_SHIFT) && (mod & KMOD_CTRL)) {
			EXPECT_EQ("ctrlshiftmodcommand", command);
			ctrlshiftmodcommand = true;
		} else if (mod & KMOD_SHIFT) {
			EXPECT_EQ("+xyz", command);
			xyz = true;
		} else if (mod & KMOD_CTRL) {
			EXPECT_EQ("+bar", command);
			bar = true;
		} else if (mod & KMOD_ALT) {
			EXPECT_EQ("somecommand +", command);
			somecommand = true;
		} else {
			EXPECT_EQ("+foo", command);
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
		const std::string& command = i->second.command;
		const int16_t mod = i->second.modifier;
		EXPECT_EQ("someothercommand +", command);
		EXPECT_TRUE(mod & KMOD_LALT) << "command " << command << " modifier wasn't parsed properly";
		EXPECT_TRUE(!(mod & KMOD_RALT)) << "command " << command << " modifier wasn't parsed properly";
		++count;
	}
	EXPECT_EQ(1, count);
}

}
