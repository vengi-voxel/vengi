/**
 * @file
 */

#include "core/Bits.h"
#include <gtest/gtest.h>

namespace core {

TEST(BitsTest, countLeadingZeros) {
	EXPECT_EQ(32, countLeadingZeros(0u));
	EXPECT_EQ(31, countLeadingZeros(1u));
	EXPECT_EQ(26, countLeadingZeros(32u));
	EXPECT_EQ(23, countLeadingZeros(256u));
	EXPECT_EQ(23, countLeadingZeros(300u));
	EXPECT_EQ(0, countLeadingZeros(0x80000000u));
}

TEST(BitsTest, countTrailingZeros) {
	EXPECT_EQ(64, countTrailingZeros(0));
	for (int bit = 0; bit < 64; ++bit) {
		EXPECT_EQ(bit, countTrailingZeros(~0ull << bit));
	}
}

TEST(BitsTest, extract1111) {
	uint32_t input = 0b1111;
	EXPECT_EQ(3u, bits(input, 0, 2));
	EXPECT_EQ(1u, bits(input, 1, 1));
	EXPECT_EQ(3u, bits(input, 1, 2));
}

TEST(BitsTest, extract1011) {
	uint32_t input = 0b1011;
	EXPECT_EQ(3u, bits(input, 0, 2));
	EXPECT_EQ(1u, bits(input, 1, 1));
	EXPECT_EQ(1u, bits(input, 1, 2));
	EXPECT_EQ(5u, bits(input, 1, 3));
}

}
