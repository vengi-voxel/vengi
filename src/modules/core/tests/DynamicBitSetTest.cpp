/**
 * @file
 */

#include "core/collection/DynamicBitSet.h"
#include "core/Common.h"
#include <gtest/gtest.h>

namespace core {

TEST(DynamicBitSetTest, testSetGet) {
	DynamicBitSet bitset(512);
	ASSERT_EQ(64u, bitset.bytes());
	bitset.set(0, true);
	bitset.set(511, true);
	EXPECT_TRUE(bitset[0]);
	EXPECT_FALSE(bitset[1]);
	EXPECT_FALSE(bitset[510]);
	EXPECT_TRUE(bitset[511]);
}

TEST(DynamicBitSetTest, testFillClear) {
	DynamicBitSet bitset(512);
	bitset.fill();
	for (size_t i = 0; i < bitset.bits(); ++i) {
		EXPECT_TRUE(bitset[i]);
	}
	bitset.clear();
	for (size_t i = 0; i < bitset.bits(); ++i) {
		EXPECT_FALSE(bitset[i]);
	}
}

TEST(DynamicBitSetTest, testCopyConstructor) {
	DynamicBitSet bitset(64);
	bitset.set(0, true);
	bitset.set(32, true);
	bitset.set(63, true);

	DynamicBitSet copy(bitset);
	EXPECT_TRUE(copy[0]);
	EXPECT_TRUE(copy[32]);
	EXPECT_TRUE(copy[63]);
	EXPECT_FALSE(copy[1]);
	EXPECT_EQ(bitset, copy);
}

TEST(DynamicBitSetTest, testCopyAssignment) {
	DynamicBitSet bitset(64);
	bitset.set(0, true);
	bitset.set(63, true);

	DynamicBitSet copy(32);
	copy = bitset;
	EXPECT_TRUE(copy[0]);
	EXPECT_TRUE(copy[63]);
	EXPECT_EQ(64u, copy.bits());
	EXPECT_EQ(bitset, copy);
}

TEST(DynamicBitSetTest, testMoveConstructor) {
	DynamicBitSet bitset(64);
	bitset.set(0, true);
	bitset.set(63, true);

	DynamicBitSet moved(core::move(bitset));
	EXPECT_TRUE(moved[0]);
	EXPECT_TRUE(moved[63]);
	EXPECT_EQ(64u, moved.bits());
	EXPECT_EQ(0u, bitset.bits());
}

TEST(DynamicBitSetTest, testMoveAssignment) {
	DynamicBitSet bitset(64);
	bitset.set(0, true);
	bitset.set(63, true);

	DynamicBitSet moved(32);
	moved = core::move(bitset);
	EXPECT_TRUE(moved[0]);
	EXPECT_TRUE(moved[63]);
	EXPECT_EQ(64u, moved.bits());
	EXPECT_EQ(0u, bitset.bits());
}

TEST(DynamicBitSetTest, testResize) {
	DynamicBitSet bitset(64);
	bitset.set(0, true);
	EXPECT_EQ(64u, bitset.bits());

	bitset.resize(128);
	EXPECT_EQ(128u, bitset.bits());
	EXPECT_TRUE(bitset[0]);

	bitset.set(127, true);
	EXPECT_TRUE(bitset[127]);
}

TEST(DynamicBitSetTest, testEquality) {
	DynamicBitSet a(64);
	DynamicBitSet b(64);
	EXPECT_EQ(a, b);

	a.set(0, true);
	EXPECT_NE(a, b);

	b.set(0, true);
	EXPECT_EQ(a, b);
}

TEST(DynamicBitSetTest, testDifferentSizes) {
	DynamicBitSet a(64);
	DynamicBitSet b(128);
	EXPECT_NE(a, b);
}

TEST(DynamicBitSetTest, testDefaultConstructor) {
	DynamicBitSet bitset;
	EXPECT_EQ(0u, bitset.bits());
	EXPECT_EQ(0u, bitset.bytes());
}

TEST(DynamicBitSetTest, testOutOfBoundsAccess) {
	DynamicBitSet bitset(64);
	// Out of bounds read should return false
	EXPECT_FALSE(bitset[64]);
	EXPECT_FALSE(bitset[100]);

	// Out of bounds write should be ignored (no crash)
	bitset.set(64, true);
	bitset.set(100, true);
	EXPECT_FALSE(bitset[64]);
}

TEST(DynamicBitSetTest, testSmallSizes) {
	DynamicBitSet bitset1(1);
	EXPECT_EQ(1u, bitset1.bits());
	bitset1.set(0, true);
	EXPECT_TRUE(bitset1[0]);

	DynamicBitSet bitset31(31);
	EXPECT_EQ(31u, bitset31.bits());
	bitset31.set(30, true);
	EXPECT_TRUE(bitset31[30]);
	EXPECT_FALSE(bitset31[31]); // out of bounds

	DynamicBitSet bitset33(33);
	EXPECT_EQ(33u, bitset33.bits());
	bitset33.set(32, true);
	EXPECT_TRUE(bitset33[32]);
}

TEST(DynamicBitSetTest, testInvert) {
	const size_t size = 100;
	DynamicBitSet bitset(size);
	bitset.set(10, true);
	bitset.set(20, true);
	bitset.set(30, true);
	bitset.invert();
	for (size_t i = 0; i < size; ++i) {
		if (i == 10 || i == 20 || i == 30) {
			EXPECT_FALSE(bitset[i]) << "Failed at index " << i;
		} else {
			EXPECT_TRUE(bitset[i]) << "Failed at index " << i;
		}
	}
}

} // namespace core
