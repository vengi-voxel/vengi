/**
 * @file
 */

#include "core/collection/BitSet.h"
#include <gtest/gtest.h>

namespace core {

TEST(BitSetTest, testSetGet) {
	BitSet<512> bitset;
	ASSERT_EQ(64u, bitset.bytes());
	bitset.set(0, true);
	bitset.set(511, true);
	EXPECT_TRUE(bitset[0]);
	EXPECT_FALSE(bitset[1]);
	EXPECT_FALSE(bitset[510]);
	EXPECT_TRUE(bitset[511]);
}

TEST(BitSetTest, testFillClear) {
	BitSet<512> bitset;
    bitset.fill();
    for (int i = 0; i < bitset.bits(); ++i) {
	    EXPECT_TRUE(bitset[i]);
    }
    bitset.clear();
    for (int i = 0; i < bitset.bits(); ++i) {
	    EXPECT_FALSE(bitset[i]);
    }
}

} // namespace core
