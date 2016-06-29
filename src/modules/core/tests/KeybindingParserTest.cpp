/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/KeybindingParser.h"

namespace core {

const char *CFG = "w +foo\n"
		"alt+a \"somecommand +\"\n"
		"CTRL+s +bar\n"
		"SHIFT+d +xyz\n";

TEST(KeybindingParserTest, testParsing) {
	KeybindingParser p(CFG);
	const BindMap& m = p.getBindings();
	ASSERT_FALSE(m.empty());
	ASSERT_EQ(0, p.invalidBindings());
	const std::size_t expected = 4;
	ASSERT_EQ(expected, m.size());
}

}
