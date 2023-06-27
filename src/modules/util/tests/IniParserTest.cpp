/**
 * @file
 */

#include "util/IniParser.h"
#include "app/tests/AbstractTest.h"
#include "io/MemoryReadStream.h"
#include "io/StringStream.h"

class IniParserTest : public app::AbstractTest {};

TEST_F(IniParserTest, testParse) {
	core::StringMap<core::String> values;
	const core::String input = "name=foo\nbgcolor=bar\nvoxels=baz\n";
	io::MemoryReadStream stream(input.c_str(), input.size());

	ASSERT_TRUE(util::parseIniSection(stream, values));
	ASSERT_TRUE(values.hasKey("name"));
	EXPECT_EQ("foo", values.find("name")->value);
}
