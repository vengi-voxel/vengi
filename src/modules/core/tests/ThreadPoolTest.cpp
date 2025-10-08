/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/concurrent/ThreadPool.h"
#include "core/collection/DynamicArray.h"
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
		pool.schedule([this] () {
			++_count;
		});
	}
	pool.shutdown(true);
	ASSERT_EQ(x, _count) << "Not all threads were executed";
}

TEST_F(ThreadPoolTest, testMultiplePushNested) {
	const int x = 100;
	core::ThreadPool pool(3);
	pool.init();
	static core::AtomicInt nestedCount = 0;
	core::DynamicArray<core::Future<void>> futures;
	for (int i = 0; i < x; ++i) {
		futures.emplace_back(pool.enqueue([this, &pool] () {
			++_count;
			auto fut = pool.enqueue([] () {
				++nestedCount;
			});
			if (fut.valid())
				fut.wait();
		}));
	}
	for (auto &fut : futures) {
		if (fut.valid()) {
			fut.wait();
		}
	}
	pool.shutdown(true);
	ASSERT_EQ(x, _count) << "Not all threads were executed";
	ASSERT_EQ(x, nestedCount) << "Not all nested threads were executed";
}

}
