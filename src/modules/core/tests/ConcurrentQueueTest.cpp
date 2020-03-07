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
	for (int i = n - 1; i >= 0; --i) {
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
	for (int i = n - 1; i >= 0; --i) {
		int v;
		ASSERT_TRUE(queue.waitAndPop(v));
		ASSERT_EQ(i, v);
	}
}

TEST_F(ConcurrentQueueTest, testPushWaitAndPopConcurrent) {
	core::ConcurrentQueue<int> queue;
	const int n = 1000;
	std::thread thread([&] () {
		for (int i = 0; i < n; ++i) {
			queue.push(i);
		}
	});
	for (int i = n - 1; i >= 0; --i) {
		int v;
		ASSERT_TRUE(queue.waitAndPop(v));
	}
	thread.join();
}

TEST_F(ConcurrentQueueTest, testPushWaitAndPopMultipleThreads) {
	core::ConcurrentQueue<int> queue;
	const int n = 1000;
	std::thread threadPush([&] () {
		for (int i = 0; i < n; ++i) {
			queue.push(i);
		}
	});
	std::thread threadPop([&] () {
		for (int i = n - 1; i >= 0; --i) {
			int v;
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

TEST_F(ConcurrentQueueTest, testSort) {
	{
		core::ConcurrentQueue<int> queue;
		queue.push(1);
		queue.push(3);
		queue.push(2);
		int val = 0;
		ASSERT_TRUE(queue.pop(val));
		EXPECT_EQ(3, val);
	}
	{
		core::ConcurrentQueue<int, std::greater<int>> queue;
		queue.push(1);
		queue.push(3);
		queue.push(2);
		int val = 0;
		ASSERT_TRUE(queue.pop(val));
		EXPECT_EQ(1, val);
	}
}

}
