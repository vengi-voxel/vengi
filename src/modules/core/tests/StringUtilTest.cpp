/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/StringUtil.h"
#include "core/tests/TestHelper.h"
#include "core/Common.h"
#include "core/StandardLib.h"

namespace core {

class StringUtilTest: public testing::Test {
};

TEST_F(StringUtilTest, addPostfixToFile) {
	EXPECT_EQ("foo-bar", core::string::addPostfixToFile("foo", "-bar"));
	EXPECT_EQ("foo-bar-bar", core::string::addPostfixToFile("foo-bar", "-bar"));
	EXPECT_EQ("foo-bar.baz", core::string::addPostfixToFile("foo.baz", "-bar"));
	EXPECT_EQ("foo/bar-baz", core::string::addPostfixToFile("foo/bar", "-baz"));
}

TEST_F(StringUtilTest, testStrncpyz) {
	char source[7] = "source";
	char target[4] = "";
	core::string::strncpyz(source, sizeof(source), target, sizeof(target));
	EXPECT_STREQ(target, "sou");
}

TEST_F(StringUtilTest, testParseHex) {
	uint8_t r, g, b, a;
	EXPECT_EQ(4, core::string::parseHex("#00112233", r, g, b, a));
	EXPECT_EQ(0, r);
	EXPECT_EQ(17, g);
	EXPECT_EQ(34, b);
	EXPECT_EQ(51, a);

	EXPECT_EQ(1, core::string::parseHex("#FF", r, g, b, a));
	EXPECT_EQ(255, r);

	EXPECT_EQ(4, core::string::parseHex("0x01020304", r, g, b, a));
	EXPECT_EQ(1, r);
	EXPECT_EQ(2, g);
	EXPECT_EQ(3, b);
	EXPECT_EQ(4, a);

	// invalid
	EXPECT_EQ(-1, core::string::parseHex("#0x01020304", r, g, b, a));
}

TEST_F(StringUtilTest, testIsInteger) {
	EXPECT_TRUE(core::string::isIntegerWithPostfix("2u"));
	EXPECT_TRUE(core::string::isIntegerWithPostfix("2"));
	EXPECT_TRUE(core::string::isIntegerWithPostfix("-2"));
	EXPECT_FALSE(core::string::isInteger("2u"));
	EXPECT_TRUE(core::string::isInteger("2"));
	EXPECT_TRUE(core::string::isInteger("-2"));
}

TEST_F(StringUtilTest, testGetBeforeToken) {
	const size_t bufSize = 32u;
	char buf[64];
	core::String::formatBuf(buf, sizeof(buf), "a = b c");
	char *p = buf;
	char *a = core::string::getBeforeToken(&p, " = ", bufSize);
	char *b = p;
	EXPECT_STREQ("a", a);
	EXPECT_STREQ("b c", b);
}

TEST_F(StringUtilTest, testPath) {
	ASSERT_EQ("bar", core::string::path("", "bar"));
	ASSERT_EQ("foo/bar", core::string::path("foo", "bar"));
	ASSERT_EQ("foo/bar/file.foo", core::string::path("foo", "bar", "file.foo"));
	ASSERT_EQ("foo/bar", core::string::path("foo/", "bar"));
	ASSERT_EQ("foo/bar", core::string::path("foo/", "/bar"));
}

TEST_F(StringUtilTest, testSanitizeDirPath) {
	EXPECT_EQ("foo/bar/", core::string::sanitizeDirPath("foo\\bar\\\\"));
	EXPECT_EQ("foo/bar/", core::string::sanitizeDirPath("foo/bar//"));
	EXPECT_EQ("foo/", core::string::sanitizeDirPath("foo"));
}

TEST_F(StringUtilTest, testUrlEncode) {
	const core::String urlEncoded = core::string::urlEncode("foobar=1236/&");
	EXPECT_EQ("foobar%3D1236%2F%26", urlEncoded);
	const core::String urlEncoded2 = core::string::urlEncode("foobar 1236/&");
	EXPECT_EQ("foobar%201236%2F%26", urlEncoded2);
}

TEST_F(StringUtilTest, testUrlPathEncode) {
	const core::String urlEncoded = core::string::urlPathEncode("/path/to#&/foo");
	EXPECT_EQ("/path/to%23%26/foo", urlEncoded) << urlEncoded;
}

TEST_F(StringUtilTest, testStripExtension) {
	EXPECT_EQ("foo", core::string::stripExtension("foo.bar"));
	EXPECT_EQ("foo.bar", core::string::stripExtension("foo.bar.foo"));
}

TEST_F(StringUtilTest, testAddFilenamePrefix) {
	EXPECT_EQ("/path/to/some/prefix-file.ext", core::string::addFilenamePrefix("/path/to/some/file.ext", "prefix-"));
}

TEST_F(StringUtilTest, testReplaceExtension) {
	EXPECT_EQ("foo.foo", core::string::replaceExtension("foo.bar", "foo"));
	EXPECT_EQ("foo.foo", core::string::replaceExtension("foo", "foo"));
}

TEST_F(StringUtilTest, testExtractDir) {
	EXPECT_EQ("/a/b/c/def/", core::string::extractDir("/a/b/c/def/foo.bar"));
	EXPECT_EQ("E:/a/b/c/def/", core::string::extractDir("E:/a/b/c/def/foo.bar"));
}

TEST_F(StringUtilTest, testExtractExtension) {
	EXPECT_EQ("bar", core::string::extractExtension("/a/b/c/def/foo.bar"));
	EXPECT_EQ("foo", core::string::extractExtension("/a/b/c/def/foo.bar.foo"));
	EXPECT_EQ("", core::string::extractExtension("/a/b/.def/foobar"));
}

TEST_F(StringUtilTest, testSplit) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString("foobar++", tokens, "+");
	ASSERT_EQ(1u, tokens.size());
	EXPECT_EQ("foobar", tokens[0]);
}

TEST_F(StringUtilTest, testSplitNoDelimiter) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString("foobar", tokens, "+");
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

TEST_F(StringUtilTest, testIsAbsolutePath) {
	EXPECT_TRUE(string::isAbsolutePath("E:\\foo\\bar\\texture\\diffuse.dds"));
	EXPECT_TRUE(string::isAbsolutePath("E:/foo/bar/texture/diffuse.dds"));
	EXPECT_TRUE(string::isAbsolutePath("/foo/bar/texture/diffuse.dds"));
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
	EXPECT_EQ("file", core::string::extractFilename("/path/to/file"));
	EXPECT_EQ("file", core::string::extractFilename("/path/to/file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("/file.extension"));
	EXPECT_EQ("file", core::string::extractFilename("file"));

	EXPECT_EQ("file", core::string::extractFilename("C:/path/to/file"));
	EXPECT_EQ("file", core::string::extractFilename("C:/path/to/file.extension"));
}

TEST_F(StringUtilTest, testExtractFilenameWithExtension) {
	EXPECT_EQ("file.extension", core::string::extractFilenameWithExtension("/path/to/file.extension"));
}

TEST_F(StringUtilTest, testHex) {
	EXPECT_EQ("000002df", core::string::toHex(735));
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

TEST_F(StringUtilTest, testReplaceAllEmpty) {
	const String str("111222333");
	EXPECT_EQ("111222333", core::string::replaceAll(str, "", ""));
}

TEST_F(StringUtilTest, testReplaceAllWithEmpty) {
	const String str("111222333");
	EXPECT_EQ("222333", core::string::replaceAll(str, "1", ""));
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

TEST_F(StringUtilTest, testMatches) {
	EXPECT_TRUE(core::string::matches("foobar", "foo*"));
	EXPECT_TRUE(core::string::matches("foobar", "?oo?*"));
	EXPECT_TRUE(core::string::matches("foobar", "*bar"));
	EXPECT_TRUE(core::string::matches("file.ext", "*.ext"));
	EXPECT_TRUE(core::string::matches("foobar", "fo?bar"));
	EXPECT_FALSE(core::string::matches("foo", "foo?"));
	EXPECT_TRUE(core::string::matches("foobar1", "foobar[123]*"));
	EXPECT_TRUE(core::string::matches("foobar14", "foobar[123]*"));
	EXPECT_TRUE(core::string::matches("foobar12", "foobar[123]*"));
	EXPECT_TRUE(core::string::matches("foobar2", "foobar[123]*"));
	EXPECT_TRUE(core::string::matches("foobar3", "foobar[123]*"));
	EXPECT_FALSE(core::string::matches("foobar4", "foobar[123]*"));
	EXPECT_FALSE(core::string::matches("foobar14", "foobar[123]"));
	EXPECT_FALSE(core::string::matches("foobar12", "foobar[123]"));
	EXPECT_TRUE(core::string::matches("foobar1", "foobar[123]"));
	EXPECT_TRUE(core::string::matches("foobar2", "foobar[123]"));
	EXPECT_TRUE(core::string::matches("foobar3", "foobar[123]"));
	EXPECT_FALSE(core::string::matches("foobar4", "foobar[123]"));
}

TEST_F(StringUtilTest, testFileMatchesMultiple) {
	EXPECT_TRUE(core::string::fileMatchesMultiple("foobar.txt", "foobar.txt"));
	EXPECT_TRUE(core::string::fileMatchesMultiple("foobar.txt", "*.txt"));
	EXPECT_TRUE(core::string::fileMatchesMultiple("foobar.tXT", "*.txt"));
	EXPECT_TRUE(core::string::fileMatchesMultiple("foobar.txt", "*.tet,*.no,*.no2,*.no3,*.txt"));
	EXPECT_FALSE(core::string::fileMatchesMultiple("foobar.txt", "tet,no,no2,no3,txt"));
	EXPECT_FALSE(core::string::fileMatchesMultiple("foobar.txt", "*.bar,*.foo"));
	EXPECT_FALSE(core::string::fileMatchesMultiple("foobar.txt", "bar,foo"));
}

}
