/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/ConcurrentQueue.h"
#include <thread>

namespace collection {

class ConcurrentQueueTest : public testing::Test {
};

TEST_F(ConcurrentQueueTest, testPushPop) {
	core::ConcurrentQueue<int> queue;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		queue.push(i);
	}
	ASSERT_EQ((int)queue.size(), n);
	for (int i = 0; i < n; ++i) {
		int v;
		ASSERT_TRUE(queue.pop(v));
		ASSERT_EQ(i, v);
	}
}

TEST_F(ConcurrentQueueTest, testPushWaitAndPop) {
	core::ConcurrentQueue<int> queue;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		queue.push(i);
	}
	ASSERT_EQ((int)queue.size(), n);
	for (int i = 0; i < n; ++i) {
		int v;
		ASSERT_TRUE(queue.waitAndPop(v));
		ASSERT_EQ(i, v);
	}
}

TEST_F(ConcurrentQueueTest, testPushWaitAndPopConcurrent) {
	const uint32_t n = 1000u;
	core::ConcurrentQueue<uint32_t> queue(n);
	std::thread thread([&] () {
		for (uint32_t i = 0; i < n; ++i) {
			queue.push(i);
		}
	});
	for (uint32_t i = 0; i < n; ++i) {
		uint32_t v;
		ASSERT_TRUE(queue.waitAndPop(v));
	}
	thread.join();
}

TEST_F(ConcurrentQueueTest, testPushWaitAndPopMultipleThreads) {
	const uint32_t n = 1000u;
	core::ConcurrentQueue<uint32_t> queue(n);
	std::thread threadPush([&] () {
		for (uint32_t i = 0u; i < n; ++i) {
			queue.push(i);
		}
	});
	std::thread threadPop([&] () {
		for (uint32_t i = 0u; i < n; ++i) {
			uint32_t v;
			ASSERT_TRUE(queue.waitAndPop(v));
		}
	});
	threadPush.join();
	threadPop.join();
}

TEST_F(ConcurrentQueueTest, testAbortWait) {
	core::ConcurrentQueue<int> queue;
	std::thread threadWait([&] () {
		int v;
		ASSERT_FALSE(queue.waitAndPop(v));
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	queue.abortWait();
	threadWait.join();
}

}
