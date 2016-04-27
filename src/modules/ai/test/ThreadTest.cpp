#include "ThreadTest.h"

class ThreadTest: public TestSuite {
};

TEST_F(ThreadTest, testThreadScheduler_scheduleAtFixedRate) {
	ai::ThreadScheduler scheduler;
	std::atomic_int countExecution(0);
	scheduler.scheduleAtFixedRate(std::chrono::milliseconds(0), std::chrono::milliseconds(10), [&] () {
		++countExecution;
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	ASSERT_GE(countExecution, 10);
	ASSERT_LE(countExecution, 30);
}

TEST_F(ThreadTest, testThreadScheduler_schedule) {
	ai::ThreadScheduler scheduler;
	std::atomic_int countExecutionOnce(0);
	scheduler.schedule(std::chrono::milliseconds(0), [&] () {
		++countExecutionOnce;
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_EQ(1, countExecutionOnce);
}
