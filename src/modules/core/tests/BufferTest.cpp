/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Buffer.h"
#include "core/Algorithm.h"

namespace core {

struct BufferStruct {
	BufferStruct() : _bar(1337) {
	}
	BufferStruct(int bar) :
			_bar(bar) {
	}
	int _bar;
};

template<size_t SIZE>
::std::ostream& operator<<(::std::ostream& ostream, const Buffer<BufferStruct, SIZE>& v) {
	int idx = 0;
	for (auto i = v.begin(); i != v.end();) {
		ostream << "'";
		ostream << i->_bar;
		ostream << "' (" << idx << ")";
		if (++i != v.end()) {
			ostream << ", ";
		}
		++idx;
	}
	return ostream;
}

TEST(BufferTest, testEmplaceBack) {
	Buffer<BufferStruct> array;
	array.emplace_back(0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testPushBack) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testClear) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.clear();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testRelease) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.release();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(0u, array.capacity()) << array;
}

TEST(BufferTest, testSort) {
	Buffer<int> array;
	array.push_back(3);
	array.push_back(5);
	array.push_back(1);
	array.push_back(11);
	array.push_back(9);
	array.sort(core::Greater<int>());
	EXPECT_EQ(1, array[0]);
	EXPECT_EQ(3, array[1]);
	EXPECT_EQ(5, array[2]);
	EXPECT_EQ(9, array[3]);
	EXPECT_EQ(11, array[4]);
}

TEST(BufferTest, testIterate) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(1));
	array.push_back(BufferStruct(2));
	array.push_back(BufferStruct(3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	int i = 1;
	for (const BufferStruct& d : array) {
		EXPECT_EQ(i++, d._bar) << array;
	}
}

TEST(BufferTest, testCopy) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(1));
	array.push_back(BufferStruct(2));
	array.push_back(BufferStruct(3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	Buffer<BufferStruct> copy(array);
	EXPECT_EQ(3u, copy.size()) << array;
	EXPECT_EQ(32u, copy.capacity()) << array;
}

TEST(BufferTest, testTriggerResize) {
	Buffer<BufferStruct, 2> array;
	array.push_back(BufferStruct(1));
	array.push_back(BufferStruct(2));
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.push_back(BufferStruct(3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(4u, array.capacity()) << array;
}

TEST(BufferTest, testResize) {
	Buffer<BufferStruct, 2> array;
	array.push_back(BufferStruct(1));
	array.push_back(BufferStruct(2));
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.resize(3);
	EXPECT_EQ(4u, array.capacity()) << array;
	ASSERT_EQ(3u, array.size()) << array;
}

TEST(BufferTest, testErase) {
	Buffer<BufferStruct> array;
	for (int i = 0; i < 128; ++i) {
		array.push_back(BufferStruct(i));
	}
	EXPECT_EQ(128u, array.size()) << array;
	EXPECT_EQ(128u, array.capacity()) << array;
	array.erase(0, 10);
	EXPECT_EQ(118u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	array.erase(1, 10);
	EXPECT_EQ(108u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	array.erase(100, 100);
	EXPECT_EQ(100u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	EXPECT_EQ(119, array[99]._bar) << array;
	array.erase(0, 99);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(119, array[0]._bar) << array;
	array.erase(0, 1);
	EXPECT_EQ(0u, array.size()) << array;
}

TEST(BufferTest, testEraseSmall) {
	Buffer<BufferStruct> array;
	array.push_back(BufferStruct(0));
	array.push_back(BufferStruct(1));
	array.push_back(BufferStruct(2));
	array.push_back(BufferStruct(3));
	array.erase(0, 1);
	EXPECT_EQ(1, array[0]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(2, array[1]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(3, array[2]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	array.erase(2, 1);
	EXPECT_EQ(1, array[0]._bar) << array;
	EXPECT_EQ(2, array[1]._bar) << array;
}

TEST(BufferTest, testAppend) {
	Buffer<BufferStruct> array;
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3)
	};
	array.append(buf, 2);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	array.append(&buf[2], 2);
	ASSERT_EQ(4u, array.size());
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);
}

TEST(BufferTest, testInsertSingle) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1)
	};
	Buffer<BufferStruct> array;
	array.reserve(2);
	array.insert(array.begin(), &buf[0], 1);
	array.insert(array.begin(), &buf[1], 1);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(1, array[0]._bar);
	EXPECT_EQ(0, array[1]._bar);
}

TEST(BufferTest, testInsertMultiple) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1)
	};
	Buffer<BufferStruct> array;
	array.reserve(2);
	array.insert(array.begin(), buf, 2);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
}

TEST(BufferTest, testInsertMiddle) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3),
		BufferStruct(4),
		BufferStruct(5)
	};
	Buffer<BufferStruct> array;
	array.reserve(32);
	array.insert(array.begin(), &buf[0], 4);
	ASSERT_EQ(4u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);

	array.insert(core::next(array.begin(), 2), buf, 6);
	ASSERT_EQ(10u, array.size());
	EXPECT_EQ(0, array[0]._bar); // previously at [0]
	EXPECT_EQ(1, array[1]._bar); // previously at [1]

	EXPECT_EQ(0, array[2]._bar); // new insert complete array - 6 entries - 0-5
	EXPECT_EQ(1, array[3]._bar);
	EXPECT_EQ(2, array[4]._bar);
	EXPECT_EQ(3, array[5]._bar);
	EXPECT_EQ(4, array[6]._bar);
	EXPECT_EQ(5, array[7]._bar);

	EXPECT_EQ(2, array[8]._bar); // previously at [2]
	EXPECT_EQ(3, array[9]._bar); // previously at [3]
}

TEST(BufferTest, testInsertMiddleInt) {
	const int buf[] = {
		0,
		1,
		2,
		3,
		4,
		5
	};
	Buffer<int> array;
	array.reserve(32);
	array.insert(array.begin(), buf, 6);
	array.insert(array.begin(), buf, 6);
	array.insert(core::next(array.begin(), 4), buf, 1);
	ASSERT_EQ(13u, array.size());
	EXPECT_EQ(0, array[0]);
	EXPECT_EQ(1, array[1]);
	EXPECT_EQ(2, array[2]);
	EXPECT_EQ(3, array[3]);
	EXPECT_EQ(0, array[4]);
	EXPECT_EQ(4, array[5]);
	EXPECT_EQ(5, array[6]);
	EXPECT_EQ(0, array[7]);
	EXPECT_EQ(1, array[8]);
	EXPECT_EQ(2, array[9]);
	EXPECT_EQ(3, array[10]);
	EXPECT_EQ(4, array[11]);
	EXPECT_EQ(5, array[12]);
}

TEST(BufferTest, testInsertMiddleBufferStruct) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3),
		BufferStruct(4),
		BufferStruct(5)
	};
	Buffer<BufferStruct> array;
	array.reserve(32);
	array.insert(array.begin(), buf, 6);
	array.insert(array.begin(), buf, 6);
	array.insert(core::next(array.begin(), 4), buf, 1);
	ASSERT_EQ(13u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);
	EXPECT_EQ(0, array[4]._bar);
	EXPECT_EQ(4, array[5]._bar);
	EXPECT_EQ(5, array[6]._bar);
	EXPECT_EQ(0, array[7]._bar);
	EXPECT_EQ(1, array[8]._bar);
	EXPECT_EQ(2, array[9]._bar);
	EXPECT_EQ(3, array[10]._bar);
	EXPECT_EQ(4, array[11]._bar);
	EXPECT_EQ(5, array[12]._bar);
}

TEST(BufferTest, testInsertIterMultiple) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3),
		BufferStruct(4),
		BufferStruct(5)
	};
	Buffer<BufferStruct> other;
	other.insert(other.begin(), buf, 6);

	Buffer<BufferStruct> array;
	array.insert(array.begin(), other.begin(), other.end());
}

TEST(BufferTest, testInsertIteratorDistance) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3),
		BufferStruct(4),
		BufferStruct(5)
	};
	Buffer<BufferStruct> other;
	other.insert(other.begin(), buf, 6);
	EXPECT_EQ(6, other.end() - other.begin());
}

TEST(BufferTest, testInsertIteratorOperatorInt) {
	const BufferStruct buf[] = {
		BufferStruct(0),
		BufferStruct(1),
		BufferStruct(2),
		BufferStruct(3),
		BufferStruct(4),
		BufferStruct(5)
	};
	Buffer<BufferStruct> other;
	other.insert(other.begin(), buf, 6);
	auto iter = other.begin();
	for (int i = 0; i < 6; ++i) {
		BufferStruct s = *iter;
		EXPECT_EQ(i, s._bar);
		s = *iter++;
		EXPECT_EQ(i, s._bar);
	}
	EXPECT_EQ(6, other.end() - other.begin());
}

}
