/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/StandardLib.h"

namespace core {

class StringUtilTest: public testing::Test {
};

TEST_F(StringUtilTest, testGetBeforeToken) {
	const size_t bufSize = 32u;
	char *buf = (char*)core_malloc(bufSize);
	char *p = buf;
	SDL_snprintf(buf, bufSize, "a = b c");
	char *a = core::string::getBeforeToken(&p, " = ", bufSize);
	char *b = p;
	EXPECT_STREQ("a", a);
	EXPECT_STREQ("b c", b);
	core_free(buf);
}

TEST_F(StringUtilTest, testFormat) {
	EXPECT_EQ("1", core::string::format("1"));
	EXPECT_EQ("10", core::string::format("%i", 10));
	EXPECT_EQ("Hello World 10 after int", core::string::format("Hello World %i after int", 10));
}

TEST_F(StringUtilTest, testUrlEncode) {
	char *urlEncoded = core::string::urlEncode("foobar=1236/&");
	EXPECT_STREQ("foobar%3D1236%2F%26", urlEncoded);
	core_free(urlEncoded);
}

TEST_F(StringUtilTest, testStripExtension) {
	EXPECT_EQ("foo", core::string::stripExtension("foo.bar"));
	EXPECT_EQ("foo.bar", core::string::stripExtension("foo.bar.foo"));
}

TEST_F(StringUtilTest, testExtractPath) {
	EXPECT_EQ("/a/b/c/def/", core::string::extractPath("/a/b/c/def/foo.bar"));
}

TEST_F(StringUtilTest, testSplit) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString("foobar++", tokens, "+");
	ASSERT_EQ(1u, tokens.size());
	EXPECT_EQ("foobar", tokens[0]);
}

TEST_F(StringUtilTest, testAppendSmall1) {
	char buf1[4] = { '\0' };
	EXPECT_EQ(&buf1[1], core::string::append(buf1, sizeof(buf1), "a"));
	EXPECT_EQ(&buf1[2], core::string::append(buf1, sizeof(buf1), "a"));
	EXPECT_EQ(&buf1[3], core::string::append(buf1, sizeof(buf1), "a"));
	EXPECT_EQ(nullptr, core::string::append(buf1, sizeof(buf1), "a"));
	ASSERT_FALSE(strcmp("aaa", buf1));
}

TEST_F(StringUtilTest, testAppendSmall2) {
	char buf1[4] = { 'a', 'a', 'a', '\0' };
	EXPECT_EQ(nullptr, core::string::append(buf1, sizeof(buf1), "a"));
	ASSERT_FALSE(strcmp("aaa", buf1));
}

TEST_F(StringUtilTest, testJoinFunc) {
	core::String test = "abcd";
	EXPECT_EQ("b,c,d,e", core::string::join(test.begin(), test.end(), ",", [] (char c) { return (char)(c + 1); }));
}

TEST_F(StringUtilTest, testCount) {
	const char *inputString = "Foo;;;Bar;;;Foo;Bar;Foo:Fas:sasdfasdf::M;;;";
	EXPECT_EQ(11, core::string::count(inputString, ';'));
}

TEST_F(StringUtilTest, testJoin) {
	core::String test = "abcd";
	EXPECT_EQ("a,b,c,d", core::string::join(test.begin(), test.end(), ","));
}

TEST_F(StringUtilTest, testJoinSingleEntry) {
	core::String test = "a";
	EXPECT_EQ("a", core::string::join(test.begin(), test.end(), ","));
}

TEST_F(StringUtilTest, testExtractFilename) {
	EXPECT_EQ("file", core::string::extractFilename("/path/to/file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("/file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("file"));
}

TEST_F(StringUtilTest, testExtractFilenameWithExtension) {
	EXPECT_EQ("file.extension", core::string::extractFilenameWithExtension("/path/to/file.extension"));
}

TEST_F(StringUtilTest, testUpperCamelCase) {
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

TEST_F(StringUtilTest, testLowerCamelCase) {
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

TEST_F(StringUtilTest, testReplaceAll) {
	const String str("111222333");
	EXPECT_EQ("222222222333", core::string::replaceAll(str, "1", "22"));
}

TEST_F(StringUtilTest, testToString) {
	const int32_t intVal = -2147483648;
	EXPECT_EQ("-2147483648", core::string::toString(intVal));
	const uint32_t unsignedIntVal = 4294967295;
	EXPECT_EQ("4294967295", core::string::toString(unsignedIntVal));
}

TEST_F(StringUtilTest, testReplaceAllEverything) {
	const String str(2, 'c');
	const String strExpected(2, 'd');
	EXPECT_EQ(strExpected, core::string::replaceAll(str, "c", "d"));
}

TEST_F(StringUtilTest, testReplaceAllBigString) {
	const String str(128, 'c');
	const String strExpected(128, 'd');
	EXPECT_EQ(strExpected, core::string::replaceAll(str, "c", "d"));
}

TEST_F(StringUtilTest, testReplaceAllChars) {
	String str("C:\\foo\\bar");
	core::string::replaceAllChars(str, '\\', '/');
	EXPECT_EQ("C:/foo/bar", str);
}

TEST_F(StringUtilTest, testReplaceAllCharsBigString) {
	String str(128, '\\');
	String strExpected(128, '/');
	core::string::replaceAllChars(str, '\\', '/');
	EXPECT_EQ(strExpected, str);
}

TEST_F(StringUtilTest, testEndsWith) {
	EXPECT_TRUE(core::string::endsWith("foobar", "bar"));
	EXPECT_FALSE(core::string::endsWith("foobar", "foo"));
}

TEST_F(StringUtilTest, testStarsWith) {
	EXPECT_FALSE(core::string::startsWith("foobar", "bar"));
	EXPECT_TRUE(core::string::startsWith("foobar", "foo"));
}

}
