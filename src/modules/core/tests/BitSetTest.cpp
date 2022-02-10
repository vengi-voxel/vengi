/**
 * @file
 */

#include "core/collection/BitSet.h"
#include <gtest/gtest.h>

namespace core {

TEST(BitSetTest, testSetGet) {
	BitSet bitset(512);
	ASSERT_EQ(64u, bitset.bytes());
	bitset.set(0, true);
	bitset.set(511, true);
	EXPECT_TRUE(bitset[0]);
	EXPECT_FALSE(bitset[1]);
	EXPECT_FALSE(bitset[510]);
	EXPECT_TRUE(bitset[511]);
}

TEST(BitSetTest, testFillClear) {
	BitSet bitset(512);
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
