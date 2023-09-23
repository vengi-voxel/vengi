/**
 * @file
 */

#include "util/IniParser.h"
#include "app/tests/AbstractTest.h"
#include "io/MemoryReadStream.h"
#include "io/StringStream.h"

class IniParserTest : public app::AbstractTest {};

TEST_F(IniParserTest, testParseSection) {
	util::IniSectionMap values;
	const core::String input = "name=foo\nbgcolor=bar\nvoxels=baz\n";
	io::MemoryReadStream stream(input.c_str(), input.size());

	ASSERT_TRUE(util::parseIniSection(stream, values));
	ASSERT_TRUE(values.hasKey("name"));
	EXPECT_EQ("foo", values.find("name")->value);
}

TEST_F(IniParserTest, testParseIni) {
	util::IniMap ini;
	const core::String input = ";comment\n[empty]\n;comment\n\n[filled]\nname=foo\nbgcolor=bar\nvoxels=baz\n";
	io::MemoryReadStream stream(input.c_str(), input.size());

	ASSERT_TRUE(util::parseIni(stream, ini));
	ASSERT_TRUE(ini.hasKey("empty"));
	ASSERT_TRUE(ini.find("empty")->second.empty());
	ASSERT_TRUE(ini.hasKey("filled"));
	ASSERT_EQ(3u, ini.find("filled")->second.size());
}
