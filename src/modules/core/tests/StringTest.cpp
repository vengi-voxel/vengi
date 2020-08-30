/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/String.h"

namespace core {

class StringTest: public testing::Test {
};

TEST_F(StringTest, testLongStringToForceAllocation) {
	String str(1024, 'c');
	EXPECT_EQ(1024u, str.size());
	str.clear();
	EXPECT_EQ(0u, str.size());
	EXPECT_EQ("", str);
	str.append("Foobar");
	EXPECT_EQ("Foobar", str);
}

TEST_F(StringTest, testCopyCtor) {
	const String str("/foo/bar");
	const String str2(str);
	EXPECT_EQ("/foo/bar", str);
	EXPECT_EQ("/foo/bar", str2);
}

TEST_F(StringTest, testCopyCtorBigString) {
	const String str(1024, 'a');
	const String str2(str);
	EXPECT_EQ(1024u, str.size());
	EXPECT_EQ(1024u, str2.size());
}

TEST_F(StringTest, testMoveAssignment) {
	String str("/foo/bar");
	String str2(core::move(str));
	EXPECT_EQ("/foo/bar", str2);
}

TEST_F(StringTest, testAssignmentOperator) {
	const String str("/foo/bar");
	String str2("foo");
	str2 = str;
	EXPECT_EQ("/foo/bar", str);
	EXPECT_EQ("/foo/bar", str2);
}

TEST_F(StringTest, testReplaceAllChars) {
	String str("abcaadefaaaa");
	str.replaceAllChars('a', ' ');
	EXPECT_EQ(" bc  def    ", str);
}

TEST_F(StringTest, testReserve) {
	String str("abcaadefaaaa");
	str.reserve(4096);
	EXPECT_EQ("abcaadefaaaa", str);
	EXPECT_EQ(12u, str.size());
}

TEST_F(StringTest, testGetAtIndex) {
	String str("abcaadefaaaa");
	EXPECT_EQ('b', str[1]);
	EXPECT_EQ('c', str[2]);
	EXPECT_EQ('f', str[7]);
}

TEST_F(StringTest, testEqual) {
	String first("a");
	String second("a");
	String third("b");
	EXPECT_EQ(first, second);
	EXPECT_NE(third, second);
	EXPECT_NE("third", second);
	EXPECT_EQ("a", second);
}

TEST_F(StringTest, testCopy) {
	String first("content");
	String second(first);
	EXPECT_EQ(first, second);
}

TEST_F(StringTest, testCompare) {
	String first("content");
	String second(first);
	String third("dontent");
	String fourth("bontent");
	EXPECT_EQ(0, first.compare(second));
	EXPECT_EQ(-1, first.compare(third));
	EXPECT_EQ(1, first.compare(fourth));

	const String str("string");
	const String str2("str");
	EXPECT_NE(0, str.compare(str2));
	EXPECT_NE(0, str2.compare(str));
}

TEST_F(StringTest, testOperatorAppend) {
	String first("content");
	first += "foo";
	EXPECT_EQ(first, "contentfoo");
}

TEST_F(StringTest, testFind) {
	String first("content");
	EXPECT_EQ(2u, first.find("n", 0u));
	EXPECT_EQ(5u, first.find("n", 3u));
}

TEST_F(StringTest, testFindBoundaries) {
	String first("content");
	EXPECT_EQ(core::String::npos, first.find("n", 1000u));
}

TEST_F(StringTest, testRFind) {
	String first("content");
	EXPECT_EQ(1u, first.find("o"));
}

TEST_F(StringTest, testFindFirstOf) {
	String first("content");
	EXPECT_EQ(2u, first.find_first_of("n"));
	EXPECT_EQ(5u, first.find_first_of("n", 4));
}

TEST_F(StringTest, testFindLastOf) {
	String first("content");
	EXPECT_EQ(6u, first.find_last_of("t"));
	EXPECT_EQ(5u, first.find_last_of("n"));
	EXPECT_EQ(4u, first.find_last_of("e"));
	EXPECT_EQ(1u, first.find_last_of("o"));
	EXPECT_EQ(0u, first.find_last_of("c"));
}

TEST_F(StringTest, testFindFirstNotOf) {
	String first("content");
	EXPECT_EQ(0u, first.find_first_not_of("n"));
	EXPECT_EQ(0u, first.find_first_not_of("o"));
	EXPECT_EQ(1u, first.find_first_not_of("c"));
	EXPECT_EQ(5u, first.find_first_not_of("e", 4));
}

TEST_F(StringTest, testSubstr) {
	String first("content");
	EXPECT_EQ("on", first.substr(1, 2));
	EXPECT_EQ("co", first.substr(0, 2));
	EXPECT_EQ("content", first.substr(0, 100));
	EXPECT_EQ("", first.substr(0, 0));
	EXPECT_EQ("content", first.substr(0, 7));
	EXPECT_EQ("conten", first.substr(0, 6));
	EXPECT_EQ("ontent", first.substr(1, 6));
}

TEST_F(StringTest, testErase) {
	String first("111_222");
	first.erase(0, 3);
	EXPECT_EQ("_222", first);
}

TEST_F(StringTest, testEraseMiddle) {
	String first(128, 'a');
	first[0] = 'b';
	first[127] = 'b';
	first.erase(1, 126);
	EXPECT_EQ("bb", first);
}

TEST_F(StringTest, testInsert) {
	String first("111_222");
	first.insert(3, "_333");
	EXPECT_EQ(11u, first.size());
	EXPECT_EQ("111_333_222", first);
}

TEST_F(StringTest, testInsertAsAppend) {
	String first("111_222");
	first.insert(7, "_333");
	EXPECT_EQ(11u, first.size());
	EXPECT_EQ("111_222_333", first);
}

TEST_F(StringTest, testReplace) {
	String first("111_222");
	first.replace(0, 3, "222222");
	EXPECT_EQ("222222_222", first);
}

TEST_F(StringTest, testReplaceBoundaries) {
	String first("111_222");
	first.replace(0, 12, "222222");
	EXPECT_EQ("222222", first);
}

TEST_F(StringTest, testToLower) {
	String first("AAABBB");
	EXPECT_EQ("aaabbb", first.toLower());
}

TEST_F(StringTest, testToUpper) {
	String first("aaabbb");
	EXPECT_EQ("AAABBB", first.toUpper());
}

TEST_F(StringTest, testIterate) {
	String first("content");
	size_t i = 0u;
	for (auto& c : first) {
		EXPECT_EQ(first[i], c);
		++i;
	}
	EXPECT_EQ(first.size(), i);
}

}
