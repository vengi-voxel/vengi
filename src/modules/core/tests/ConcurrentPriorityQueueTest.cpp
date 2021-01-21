/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/ConcurrentPriorityQueue.h"
#include <thread>

namespace collection {

class ConcurrentPriorityQueueTest : public testing::Test {
};

TEST_F(ConcurrentPriorityQueueTest, testPushPop) {
	const int n = 1000;
	core::ConcurrentPriorityQueue<int> queue(n);
	for (int i = 0; i < n; ++i) {
		queue.push(i);
	}
	ASSERT_EQ((int)queue.size(), n);
	for (int i = n - 1; i >= 0; --i) {
		int v;
		ASSERT_TRUE(queue.pop(v));
		ASSERT_EQ(i, v);
	}
}

TEST_F(ConcurrentPriorityQueueTest, testPushWaitAndPop) {
	const int n = 1000;
	core::ConcurrentPriorityQueue<int> queue(n);
	for (int i = 0; i < n; ++i) {
		queue.push(i);
	}
	ASSERT_EQ((int)queue.size(), n);
	for (int i = n - 1; i >= 0; --i) {
		int v;
		ASSERT_TRUE(queue.waitAndPop(v));
		ASSERT_EQ(i, v);
	}
}

TEST_F(ConcurrentPriorityQueueTest, testPushWaitAndPopConcurrent) {
	const uint32_t n = 1000u;
	core::ConcurrentPriorityQueue<uint32_t> queue(n);
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

TEST_F(ConcurrentPriorityQueueTest, testPushWaitAndPopMultipleThreads) {
	const uint32_t n = 1000u;
	core::ConcurrentPriorityQueue<uint32_t> queue(n);
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

TEST_F(ConcurrentPriorityQueueTest, testAbortWait) {
	core::ConcurrentPriorityQueue<int> queue;
	std::thread threadWait([&] () {
		int v;
		ASSERT_FALSE(queue.waitAndPop(v));
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	queue.abortWait();
	threadWait.join();
}

TEST_F(ConcurrentPriorityQueueTest, testSort) {
	{
		core::ConcurrentPriorityQueue<int> queue(3);
		queue.push(1);
		queue.push(3);
		queue.push(2);
		int val = 0;
		ASSERT_TRUE(queue.pop(val));
		EXPECT_EQ(3, val);
	}
	{
		core::ConcurrentPriorityQueue<int, std::greater<int>> queue;
		queue.push(1);
		queue.push(3);
		queue.push(2);
		int val = 0;
		ASSERT_TRUE(queue.pop(val));
		EXPECT_EQ(1, val);
	}
}

}
