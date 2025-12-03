/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Unicode.h"

namespace core {
namespace unicode {

class UnicodeTest: public testing::Test {
};

TEST_F(UnicodeTest, testCharLengthUtf16) {
	EXPECT_EQ(1u, charLengthUtf16("a"));
	EXPECT_EQ(1u, charLengthUtf16("€")); // 3 bytes in UTF-8, 1 char in UTF-16
	EXPECT_EQ(2u, charLengthUtf16("𐍈")); // 4 bytes in UTF-8, 2 chars in UTF-16 (surrogate pair)
}

TEST_F(UnicodeTest, testCharLengthUtf8) {
	EXPECT_EQ(1u, charLengthUtf8("a"));
	EXPECT_EQ(1u, charLengthUtf8("€"));
	EXPECT_EQ(1u, charLengthUtf8("𐍈"));
}

TEST_F(UnicodeTest, testToUtf16) {
	const char *utf8 = "a€𐍈";
	uint16_t utf16[16];
	const int len = toUtf16(utf8, 8, utf16, sizeof(utf16) / sizeof(utf16[0]));
	EXPECT_EQ(4, len);
	EXPECT_EQ('a', utf16[0]);
	EXPECT_EQ(0x20AC, utf16[1]);
	EXPECT_EQ(0xD800, utf16[2]);
	EXPECT_EQ(0xDF48, utf16[3]);
}

TEST_F(UnicodeTest, testToUtf8) {
	const uint16_t utf16[] = {'a', 0x20AC, 0xD800, 0xDF48};
	char utf8[16];
	const int len = toUtf8(utf16, 4, utf8, sizeof(utf8));
	EXPECT_EQ(8, len);
	EXPECT_STREQ("a€𐍈", utf8);
}

}
}
