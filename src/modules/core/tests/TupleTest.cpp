/**
 * @file
 */

#include "core/Tuple.h"
#include <gtest/gtest.h>

namespace core {

TEST(TupleTest, testTuple) {
	core::Tuple<int, float, char> tuple(1, 2.0f, 'a');
	EXPECT_EQ(1, core::get<0>(tuple));
	EXPECT_EQ(2.0f, core::get<1>(tuple));
	EXPECT_EQ('a', core::get<2>(tuple));
}

} // namespace core
