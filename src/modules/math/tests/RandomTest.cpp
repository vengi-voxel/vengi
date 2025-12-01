/**
 * @file
 */

#include "math/Random.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/Array.h"

namespace math {

class RandomTest : public app::AbstractTest {};

TEST_F(RandomTest, testRandom) {
	Random r(0);
	EXPECT_EQ(0u, r.seed());
	r.setSeed(1);
	EXPECT_EQ(1u, r.seed());
	EXPECT_GE(r.randomf(), 0.0f);
	EXPECT_LE(r.randomf(), 1.0f);
	EXPECT_GE(r.random(0, 10), 0);
	EXPECT_LE(r.random(0, 10), 10);
}

TEST_F(RandomTest, testRandomBinomial) {
	Random r(0);
	float val = r.randomBinomial(10.0f);
	EXPECT_GE(val, -10.0f);
	EXPECT_LE(val, 10.0f);
}

TEST_F(RandomTest, testRandomElement) {
	Random r(0);
	core::Array<int, 5> v = {1, 2, 3, 4, 5};
	auto it = r.randomElement(v.begin(), v.end());
	EXPECT_NE(it, v.end());
	EXPECT_GE(*it, 1);
	EXPECT_LE(*it, 5);
}

} // namespace math
