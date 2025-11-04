/**
 * @file
 */

#include "app/Async.h"
#include "app/tests/AbstractTest.h"

namespace app {

class AsyncTest : public app::AbstractTest {};

TEST_F(AsyncTest, testForParallel) {
	constexpr int size = 512;
	int buf[size + 1];
	buf[size] = -1;
	app::for_parallel(0, size, [&buf](int start, int end) {
		for (int i = start; i < end; ++i) {
			buf[i] = i;
		}
	});
	for (int i = 0; i < size; ++i) {
		ASSERT_EQ(buf[i], i);
	}
	// sentinel
	ASSERT_EQ(buf[size], -1);
}

TEST_F(AsyncTest, testSort) {
	core::Array<int, 8> foo{{1, 5, 3, 7, 8, 10, 100, -100}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(-100, foo[0]);
	EXPECT_EQ(1, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(5, foo[3]);
	EXPECT_EQ(7, foo[4]);
	EXPECT_EQ(8, foo[5]);
	EXPECT_EQ(10, foo[6]);
	EXPECT_EQ(100, foo[7]);
}

TEST_F(AsyncTest, testSort1) {
	core::Array<int, 1> foo{{1}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(1, foo[0]);
}

TEST_F(AsyncTest, testSort2) {
	core::Array<int, 2> foo{{2, 1}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
}

TEST_F(AsyncTest, testEmpty) {
	core::Array<int, 2> foo{{0, -1}};
	app::sort_parallel(foo.begin(), foo.begin(), core::Less<int>());
	EXPECT_EQ(0, foo[0]);
	EXPECT_EQ(-1, foo[1]);
}

TEST_F(AsyncTest, testPartially) {
	core::Array<int, 5> foo{{0, -1, -2, -4, -6}};
	app::sort_parallel(foo.begin(), core::next(foo.begin(), 2), core::Less<int>());
	EXPECT_EQ(-1, foo[0]);
	EXPECT_EQ( 0, foo[1]);
	EXPECT_EQ(-2, foo[2]);
	EXPECT_EQ(-4, foo[3]);
	EXPECT_EQ(-6, foo[4]);
}

} // namespace app
