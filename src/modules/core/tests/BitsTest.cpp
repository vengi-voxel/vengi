/**
 * @file
 */

#include "core/Bits.h"
#include <gtest/gtest.h>

namespace core {

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
