/**
 * @file
 */

#include "app/Async.h"
#include "app/tests/AbstractTest.h"
#include <glm/vec3.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace app {

class AsyncTest : public app::AbstractTest {
public:
	AsyncTest() : app::AbstractTest(8) {
	}
};

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

TEST_F(AsyncTest, testSortLarge) {
	constexpr int size = 10240;
	core::Array<glm::vec3, size> foo;
	// Fill with reverse order
	for (size_t i = 0; i < foo.size(); ++i) {
		foo[i] = glm::vec3(size - i, size, i);
	}
	glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);
	app::sort_parallel(foo.begin(), foo.end(), [&cameraPos] (const glm::vec3& lhs, const glm::vec3& rhs) {
		return glm::distance2(lhs, cameraPos) < glm::distance2(rhs, cameraPos);
	});
}

TEST_F(AsyncTest, testSortLargeRandom) {
	constexpr int size = 512;
	core::Array<int, size> foo;
	// Fill with pseudo-random data
	for (int i = 0; i < size; ++i) {
		foo[i] = (i * 13 + 7) % 1000;
	}
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	// Verify sorted
	for (int i = 1; i < size; ++i) {
		EXPECT_LE(foo[i-1], foo[i]) << "at index " << i;
	}
}

TEST_F(AsyncTest, testSortDuplicates) {
	core::Array<int, 20> foo{{5, 3, 5, 1, 3, 5, 2, 1, 3, 5, 4, 5, 3, 2, 1, 5, 3, 4, 2, 1}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(1, foo[1]);
	EXPECT_EQ(1, foo[2]);
	EXPECT_EQ(1, foo[3]);
	EXPECT_EQ(2, foo[4]);
	EXPECT_EQ(2, foo[5]);
	EXPECT_EQ(2, foo[6]);
	EXPECT_EQ(3, foo[7]);
	EXPECT_EQ(3, foo[8]);
	EXPECT_EQ(3, foo[9]);
	EXPECT_EQ(3, foo[10]);
	EXPECT_EQ(3, foo[11]);
	EXPECT_EQ(4, foo[12]);
	EXPECT_EQ(4, foo[13]);
	EXPECT_EQ(5, foo[14]);
	EXPECT_EQ(5, foo[15]);
	EXPECT_EQ(5, foo[16]);
	EXPECT_EQ(5, foo[17]);
	EXPECT_EQ(5, foo[18]);
	EXPECT_EQ(5, foo[19]);
}

TEST_F(AsyncTest, testSortAlreadySorted) {
	core::Array<int, 10> foo{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	for (int i = 0; i < 10; ++i) {
		EXPECT_EQ(i + 1, foo[i]);
	}
}

TEST_F(AsyncTest, testSortNegatives) {
	core::Array<int, 8> foo{{-5, 3, -10, 7, -1, 0, -100, 50}};
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(-100, foo[0]);
	EXPECT_EQ(-10, foo[1]);
	EXPECT_EQ(-5, foo[2]);
	EXPECT_EQ(-1, foo[3]);
	EXPECT_EQ(0, foo[4]);
	EXPECT_EQ(3, foo[5]);
	EXPECT_EQ(7, foo[6]);
	EXPECT_EQ(50, foo[7]);
}

TEST_F(AsyncTest, testSortDescending) {
	core::Array<int, 8> foo{{1, 5, 3, 7, 8, 10, 100, -100}};
	app::sort_parallel(foo.begin(), foo.end(), core::Greater<int>());
	EXPECT_EQ(100, foo[0]);
	EXPECT_EQ(10, foo[1]);
	EXPECT_EQ(8, foo[2]);
	EXPECT_EQ(7, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(3, foo[5]);
	EXPECT_EQ(1, foo[6]);
	EXPECT_EQ(-100, foo[7]);
}

TEST_F(AsyncTest, testSortMedium) {
	constexpr int size = 128;
	core::Array<int, size> foo;
	// Fill with pattern
	for (int i = 0; i < size; ++i) {
		foo[i] = (i % 2 == 0) ? size - i : i;
	}
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	// Verify sorted
	for (int i = 1; i < size; ++i) {
		EXPECT_LE(foo[i-1], foo[i]) << "at index " << i;
	}
}

TEST_F(AsyncTest, testSortAllSame) {
	core::Array<int, 50> foo;
	for (int i = 0; i < 50; ++i) {
		foo[i] = 42;
	}
	app::sort_parallel(foo.begin(), foo.end(), core::Less<int>());
	for (int i = 0; i < 50; ++i) {
		EXPECT_EQ(42, foo[i]);
	}
}

} // namespace app
