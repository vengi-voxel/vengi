/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/concurrent/ThreadPool.h"
#include "core/concurrent/Atomic.h"

namespace core {

class ThreadPoolTest: public testing::Test {
public:
	core::AtomicInt _count;
	bool _executed = false;

	void SetUp() override {
		_count = 0;
		_executed = false;
	}
};

TEST_F(ThreadPoolTest, testPush) {
	core::ThreadPool pool(1);
	pool.init();
	auto future = pool.enqueue([this] () {
		_executed = true;
	});
	future.get();
	ASSERT_TRUE(_executed) << "Thread wasn't executed";
}

TEST_F(ThreadPoolTest, testMultiplePush) {
	const int x = 1000;
	core::ThreadPool pool(2);
	pool.init();
	for (int i = 0; i < x; ++i) {
		pool.enqueue([this] () {
			++_count;
		});
	}
	pool.shutdown(true);
	ASSERT_EQ(x, _count) << "Not all threads were executed";
}

}
