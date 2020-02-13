/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/DynamicArray.h"
#include "core/String.h"

namespace core {

struct DynamicArrayStruct {
	DynamicArrayStruct(const core::String &foo, int bar) :
			_foo(foo), _bar(bar) {
	}
	core::String _foo;
	int _bar;
};

template<size_t SIZE>
::std::ostream& operator<<(::std::ostream& ostream, const DynamicArray<DynamicArrayStruct, SIZE>& v) {
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

TEST(DynamicArrayTest, testEmplaceBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.emplace_back("", 0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testPushBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testClear) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.clear();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testRelease) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.release();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(0u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testIterate) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	int i = 1;
	for (const DynamicArrayStruct& d : array) {
		EXPECT_EQ(i++, d._bar) << array;
	}
}

TEST(DynamicArrayTest, testCopy) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	DynamicArray<DynamicArrayStruct> copy(array);
	EXPECT_EQ(3u, copy.size()) << array;
	EXPECT_EQ(32u, copy.capacity()) << array;
}

TEST(DynamicArrayTest, testTriggerResize) {
	DynamicArray<DynamicArrayStruct, 2> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(4u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testErase) {
	DynamicArray<DynamicArrayStruct> array;
	for (int i = 0; i < 128; ++i) {
		array.push_back(DynamicArrayStruct("", i));
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

TEST(DynamicArrayTest, testEraseSmall) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct(core::String(1024, 'a'), 0));
	array.push_back(DynamicArrayStruct(core::String(1024, 'b'), 1));
	array.push_back(DynamicArrayStruct(core::String(4096, 'c'), 2));
	array.push_back(DynamicArrayStruct(core::String(1337, 'd'), 3));
	array.erase(0, 1);
	EXPECT_EQ(1, array[0]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(2, array[1]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(3, array[2]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	array.erase(2, 1);
	EXPECT_EQ(1, array[0]._bar) << array;
	EXPECT_EQ(2, array[1]._bar) << array;
}

}
