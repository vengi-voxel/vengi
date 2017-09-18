/**
 * @file
 */

#include "AbstractTest.h"
#include "core/String.h"

namespace core {

class StringTest: public AbstractTest {
};

TEST_F(StringTest, testEraseAllSpaces) {
	ASSERT_EQ("", core::string::eraseAllSpaces("   "));
	ASSERT_EQ(",", core::string::eraseAllSpaces("  ,  "));
}

TEST_F(StringTest, testExtractFilename) {
	ASSERT_EQ("file", core::string::extractFilename("/path/to/file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("/file.extension"));
	ASSERT_EQ("file", core::string::extractFilename("file"));
}

TEST_F(StringTest, testCutAfterFirstMatch) {
	std::string_view test("filename.ext");
	ASSERT_EQ("filename", core::string::cutAfterFirstMatch(test, "."));
}

TEST_F(StringTest, testCutAfterFirstMatchString) {
	std::string test("filename.ext");
	ASSERT_EQ("filename", core::string::cutAfterFirstMatch(test, "."));
}

TEST_F(StringTest, testToLower) {
	std::string test("FILENAME.EXT");
	ASSERT_EQ("filename.ext", core::string::toLower(test));
}

TEST_F(StringTest, testUpperCamelCase) {
	EXPECT_EQ("FooBar", core::string::upperCamelCase("foo_bar"));
	EXPECT_EQ("FooBar", core::string::upperCamelCase("FooBar"));
	EXPECT_EQ("", core::string::upperCamelCase("_"));
	EXPECT_EQ("", core::string::upperCamelCase("__"));
	EXPECT_EQ("", core::string::upperCamelCase("___"));
	EXPECT_EQ("A", core::string::upperCamelCase("__a"));
	EXPECT_EQ("AA", core::string::upperCamelCase("_a_a"));
	EXPECT_EQ("AA", core::string::upperCamelCase("a_a_"));
	EXPECT_EQ("AA", core::string::upperCamelCase("a__a"));
	EXPECT_EQ("AAA", core::string::upperCamelCase("a_a_a"));
	EXPECT_EQ("Foobar", core::string::upperCamelCase("Foobar"));
	EXPECT_EQ("FooBar", core::string::upperCamelCase("_foo_bar_"));
	EXPECT_EQ("FooBar", core::string::upperCamelCase("_foo__bar_"));
	EXPECT_EQ("FooBAr", core::string::upperCamelCase("_foo__b_ar_"));
	EXPECT_EQ("FooBAr", core::string::upperCamelCase("___foo___b__ar__"));
}

TEST_F(StringTest, testLowerCamelCase) {
	EXPECT_EQ("fooBar", core::string::lowerCamelCase("foo_bar"));
	EXPECT_EQ("fooBar", core::string::lowerCamelCase("FooBar"));
	EXPECT_EQ("", core::string::lowerCamelCase("_"));
	EXPECT_EQ("", core::string::lowerCamelCase("__"));
	EXPECT_EQ("", core::string::lowerCamelCase("___"));
	EXPECT_EQ("a", core::string::lowerCamelCase("__a"));
	EXPECT_EQ("aA", core::string::lowerCamelCase("_a_a"));
	EXPECT_EQ("aA", core::string::lowerCamelCase("a_a_"));
	EXPECT_EQ("aA", core::string::lowerCamelCase("a__a"));
	EXPECT_EQ("aAA", core::string::lowerCamelCase("a_a_a"));
	EXPECT_EQ("foobar", core::string::lowerCamelCase("Foobar"));
	EXPECT_EQ("fooBar", core::string::lowerCamelCase("_foo_bar_"));
	EXPECT_EQ("fooBar", core::string::lowerCamelCase("_foo__bar_"));
	EXPECT_EQ("fooBAr", core::string::lowerCamelCase("_foo__b_ar_"));
	EXPECT_EQ("fooBAr", core::string::lowerCamelCase("___foo___b__ar__"));
}

}
