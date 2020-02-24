/**
 * @file
 */

#include "AbstractTest.h"
#include "core/concurrent/ReadWriteLock.h"
#include <future>

namespace core {

class ReadWriteLockTest: public AbstractTest {
protected:
	core::ReadWriteLock _rwLock {"test"};
	int _value = 0;
	const int limit { 100000 };

	int read(int loopLimit) {
		int n = 0;
		for (int i = 0; i < loopLimit; ++i) {
			core::ScopedReadLock scoped(_rwLock);
			if (_value >= 0) {
				++n;
			}
		}
		return n;
	}

	void write(int limit) {
		for (int i = 0; i < limit; ++i) {
			core::ScopedWriteLock scoped(_rwLock);
			++_value;
		}
	}
};

TEST_F(ReadWriteLockTest, testSameReadersThanWriters) {
	int n1 = 0, n2 = 0;
	auto futureRead1 = std::async(std::launch::async, [&] {n1 += read(limit);});
	auto futureRead2 = std::async(std::launch::async, [&] {n2 += read(limit);});
	auto futureWrite1 = std::async(std::launch::async, [=] {write(limit);});
	auto futureWrite2 = std::async(std::launch::async, [=] {write(limit);});
	futureRead1.wait();
	futureRead2.wait();
	futureWrite1.wait();
	futureWrite2.wait();
	EXPECT_EQ(_value, limit * 2);
	EXPECT_EQ(n1, limit);
	EXPECT_EQ(n2, limit);
}

TEST_F(ReadWriteLockTest, testMoreReadersThanWriters) {
	int n1 = 0, n2 = 0, n3 = 0;
	auto futureRead1 = std::async(std::launch::async, [&] {n1 += read(limit);});
	auto futureRead2 = std::async(std::launch::async, [&] {n2 += read(limit);});
	auto futureRead3 = std::async(std::launch::async, [&] {n3 += read(limit);});
	auto futureWrite = std::async(std::launch::async, [=] {write(limit);});
	futureRead1.wait();
	futureRead2.wait();
	futureRead3.wait();
	futureWrite.wait();
	EXPECT_EQ(_value, limit);
	EXPECT_EQ(n1, limit);
	EXPECT_EQ(n2, limit);
	EXPECT_EQ(n3, limit);
}

TEST_F(ReadWriteLockTest, testMoreWritersThanReaders) {
	int n1 = 0;
	auto futureRead1 = std::async(std::launch::async, [&] {n1 += read(limit);});
	auto futureWrite1 = std::async(std::launch::async, [=] {write(limit);});
	auto futureWrite2 = std::async(std::launch::async, [=] {write(limit);});
	auto futureWrite3 = std::async(std::launch::async, [=] {write(limit);});
	futureRead1.wait();
	futureWrite1.wait();
	futureWrite2.wait();
	futureWrite3.wait();
	EXPECT_EQ(n1, limit);
}

}
